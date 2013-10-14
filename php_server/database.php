<?php

include 'glip/lib/glip.php'; 


class TreeBuilder {
	private $rootTree;
	private $repo;
	private $newTrees;

	public function __construct($rootTree) {
		$this->rootTree = $rootTree;
		$this->repo = $rootTree->repo;
		$this->newTrees = array();
	}

	public function write() {
		foreach ($this->newTrees as $tree)
			$tree->write();
		$newTrees = array();
	}

	public function updateNode($path, $mode, $object)
    {
		$this->_updateNode($this->rootTree, $path, $mode, $object);
    }

	public function _updateNode($tree, $path, $mode, $object)
    {
        if (!is_array($path))
            $path = explode('/', $path);
        $name = array_shift($path);
        if (count($path) == 0)
        {
            /* create leaf node */
            if ($mode)
            {
                $node = new stdClass;
                $node->mode = $mode;
                $node->name = $name;
                $node->object = $object;
                $node->is_dir = !!($mode & 040000);

                $tree->nodes[$node->name] = $node;
            }
            else
                unset($tree->nodes[$name]);
        }
        else
        {
			$wasInList = false;
            /* descend one level */
            if (isset($tree->nodes[$name]))
            {
                $node = $tree->nodes[$name];
                if (!$node->is_dir)
                    throw new GitTreeInvalidPathError;
                $subtree = $this->_findSubTree($node->object);
                if ($subtree == NULL)
					$subtree = clone $this->repo->getObject($node->object);
				else
					$wasInList = true;
            }
            else
            {
                /* create new tree */
                $subtree = new GitTree($this->repo);

                $node = new stdClass;
                $node->mode = 040000;
                $node->name = $name;
                $node->is_dir = TRUE;

                $tree->nodes[$node->name] = $node;
            }
            $this->_updateNode($subtree, $path, $mode, $object);

            $subtree->rehash();
            $node->object = $subtree->getName();

            if (!$wasInList)
				$this->newTrees[] = $subtree;
        }
    }
   
    private function _findSubTree($name) {
		foreach ($this->newTrees as $tree) {
			if ($tree->getName() == $name)
				return $tree;
		}
		return NULL;
    }
}


class MessageFeeder {
	public $repository;
	public $dir;

	public function __construct($repoPath) {
		$this->dir = $repoPath;
		$this->init();
		$this->repository = new Git($repoPath);
	}

	public function init() {
		if (file_exists($this->dir))
			return false;
		mkdir($this->dir);
		mkdir($this->dir."/objects");
		mkdir($this->dir."/objects/pack");
		mkdir($this->dir."/refs");
		mkdir($this->dir."/refs/heads/");

		# set HEAD to master branch
		$f = fopen($this->dir."/HEAD", 'cb');
		flock($f, LOCK_EX);
		ftruncate($f, 0);
		$data = "ref: refs/heads/master";
		fwrite($f, $data);
		fclose($f);
		return true;
	}

	private function findLocalBranch($tiphex) {
		foreach (new DirectoryIterator($this->dir."/refs/heads/") as $file) {
			if($file->isDot())
				continue;
			$path = $this->dir."/refs/heads/".$file->getFilename();
			if (file_get_contents($path) == $tiphex)
				return $file->getFilename();
		}
		
		$path = sprintf('%s/packed-refs', $this->dir);
		if (file_exists($path))
		{
			$branch = NULL;
			$f = fopen($path, 'rb');
			flock($f, LOCK_SH);
			while ($branch === NULL && ($line = fgets($f)) != FALSE)
			{
				if ($line{0} == '#')
					continue;
				$parts = explode(' ', trim($line));
				if (count($parts) == 2 && $parts[0] == $tiphex) {
					if (preg_match('#^refs/heads/(.*)$#', $parts[1], $m))
						$branch = $m;
				}
			}
			fclose($f);
			if ($branch !== NULL)
				return $branch;

			throw new Exception(sprintf('no such branch with tip: %s', $tiphex));
		}
	}

	/*! Returns true if $head is a branch name else $head is the hex id of the detached head. */
	public function getHead(&$head) {
		if (!file_exists($this->dir."/HEAD"))
			throw new Exception('HEAD not found');
		$f = fopen($this->dir."/HEAD", 'rb');
		flock($f, LOCK_SH);
		if (($line = fgets($f)) == FALSE)
			throw new Exception('No entry in HEAD');
		if (preg_match('#^ref: refs/heads/(.*)$#', $line, $head))
			return true;
		$head = $line;
		return false;
	}

	public function setBranchTip($branchName, $commit) {
		$f = fopen($this->dir."/refs/heads/$branchName", 'cb');
		flock($f, LOCK_SH);
		ftruncate($f, 0);
		fwrite($f, sha1_hex($commit));
		fclose($f);
	}

	public function setHead($commitHex) {
		if (!file_exists($this->dir."/HEAD"))
			throw new Exception('HEAD not found');
		$f = fopen($this->dir."/HEAD", 'rb');
		flock($f, LOCK_SH);
		if (($line = fgets($f)) == FALSE)
			throw new Exception('No entry in HEAD');
		ftruncate($f, 0);
		fwrite($f, $commitHex);
		fclose($f);

		$branch = NULL;
		if ($line == "ref: refs/heads/master")
			$branch = master;
		else
			$branch = findLocalBranch($line);

		if ($branch !== NULL) {
			$f = fopen($this->dir."/refs/heads/$branch", 'rb');
			flock($f, LOCK_SH);
			ftruncate($f, 0);
			fwrite($f, $commitHex);
			fclose($f);
		}
	}

	// unused
	public function getRootTree($branch) {
		$rootTree = NULL;
		try {
			$tip = $this->repository->getTip($branch);
			$rootCommit = $this->repository->getObject($tip);
			$rootTree = clone $rootCommit->tree;
		} catch (Exception $e) {
			$rootTree = new GitTree($this->repository);
		}
		return $rootTree;
	}

	public function addMessage($header, $body, $attachments) {
		$branch = "master";

		# get root tree and last commit
		$rootTree = NULL;
		$tip = "";
		try {
			$tip = $this->repository->getTip($branch);
			$rootCommit = $this->repository->getObject($tip);
			$rootTree = clone $this->repository->getObject($rootCommit->tree);
		} catch (Exception $e) {
			$rootTree = new GitTree($this->repository);
			$rootTree->write();
		}

		# find message name
		$hash = hash_init('sha1');
		hash_update($hash, $header);
		hash_update($hash, $body);
		foreach ($attachments as $attachment)
			hash_update($hash, $attachment);
		$messageName = sha1_hex(hash_final($hash, TRUE));

		$treeBuilder = new TreeBuilder($rootTree);
		# build new tree
		$headerObj = $this->writeBlob($header);
		$treeBuilder->updateNode("messages/$messageName/header",
			100644, $headerObj->getName());

		$bodyObj = $this->writeBlob($body);
		$treeBuilder->updateNode("messages/$messageName/body",
			100644, $bodyObj->getName());
		$attachmentsObj = array();

		foreach ($attachments as $attachment) {
			$attachmentObj = $this->writeBlob($attachment->data);
			$treeBuilder->updateNode(
				"messages/$messageName/".$attachment->name, 100644,
				$attachmentObj->getName());
		}

		$treeBuilder->write();

		# write tree and new sub trees
		$rootTree->rehash();
		$rootTree->write();

		# write commit
		$parents = array();
		if (strlen($tip) > 0)
			$parents[] = $tip;

		$commit = $this->commitMessage($rootTree->getName(), $parents);
		$commit->rehash();

		$this->setBranchTip($branch, $commit->getName());
	}

	private function writeBlob($data) {
		$object = new GitBlob($this->repository);
		$object->data = $data;
		$object->rehash();
		$object->write();
		return $object;
	}

	private function commitMessage($tree, $parents) {
		$commit = new GitCommit($this->repository);
		$commit->tree = $tree;
		$commit->parents = $parents;

		$commitStamp = new GitCommitStamp;
		$commitStamp->name = "no name";
		$commitStamp->email = "no mail";
		$commitStamp->time = time();
		$commitStamp->offset = 0;

		$commit->author = $commitStamp;
		$commit->committer = $commitStamp;
		$commit->summary = "Message";
		$commit->detail = "";

		$commit->rehash();
		$commit->write();

		return $commit;
	}
}


class MessageSyncManager {
	public $repository;

	public function __construct($repository) {
		$this->repository = $repository;
	}

	public function printObject($name) {
		$object = $this->repository->getObject($name);
		if ($object->getType() == Git::OBJ_TREE) {
			echo "Object Tree: ".sha1_hex($object->getName())."<br>";
			foreach ($object->nodes as $node)
				echo "-node: ".$node->name."<br>";
		} else if ($object->getType() == Git::OBJ_COMMIT) {
			echo "Object Commit: ".sha1_hex($object->getName())."<br>";
		} else if ($object->getType() == Git::OBJ_BLOB) {
			echo "Object Blob: ".sha1_hex($object->getName())."<br>";
		}
	}

	public function listTreeOjects($treeName) {
		$objects = array();
		$objects[] = $treeName;
		$treesQueue = array();
		$treesQueue[] = $treeName;
		while (true) {
			$currentTree = array_pop($treesQueue);
			if ($currentTree == NULL)
				break;
			$treeObject = $this->repository->getObject($currentTree);
			foreach ($treeObject->nodes as $node)
			{
				$objects[] = $node->object;
				if ($node->is_dir)
					$treesQueue[] = $node->object;
			}
		}
		#debug
		/*echo "List:<br>";
		foreach ($objects as $object)
			$this->printObject($object);*/
		return $objects;
    }

	public function findMissingObjects($listOld, $listNew) {
		$missing = array();

		sort($listOld);
		sort($listNew);

		$a = $b = 0;
		while ($a < count($listOld) || $b < count($listNew))
		{
			if ($a < count($listOld) && $b < count($listNew))
				$cmp = strcmp($listOld[$a], $listNew[$b]);
			else
				$cmp = 0;
			if ($b >= count($listNew) || $cmp < 0)
			{
				$a++;
			}
			else if ($a >= count($listOld) || $cmp > 0)
			{
				$missing[] = $listNew[$b];
				$b++;
			}
			else
			{
				$a++;
				$b++;
			}
		}

		return $missing;
    }

    
	private function collectMissingBlobs($commitStop, $commitLast, $type = -1) {
		$blobs = array();
		$handledCommits = array();
		$commits = array();
		$commits[] = $commitLast;
		while (true) {
			$currentCommit = array_pop($commits);
			if ($currentCommit == NULL)
				break;
			if ($currentCommit == $commitStop)
				continue;
			if (in_array($currentCommit, $handledCommits))
				continue;
			$handledCommits[] = $currentCommit;

			$commitObject = $this->repository->getObject($currentCommit);
			$parents = $commitObject->parents;
			foreach ($parents as $parent) {
				$parentObject = $this->repository->getObject($parent);
				
				$diffs = $this->findMissingObjects($this->listTreeOjects($parentObject->tree),
					$this->listTreeOjects($commitObject->tree));
				foreach ($diffs as $diff) {
					$object = $this->repository->getObject($diff);
					if ($type <= 0 || $object->getType() == $type) {
						if (!in_array($diff, $blobs)) {
							$blobs[] = $diff;
						}
					}
				}
				$commits[] = $parent;
			}
		}
		foreach ($handledCommits as $handledCommits)
			$blobs[] = $handledCommits;

		return $blobs;
	}

	public function packMissingDiffs($branch, $commitOldest, &$commitLatest, $type) {
		if ($commitLatest == NULL)
			$commitLatest = $this->repository->getTip($branch);

		$blobs = $this->collectMissingBlobs($commitOldest, $commitLatest, $type);
		return $this->packObjects($blobs);
	}

	public function packObjects($objects) {
		$pack;
		foreach ($objects as $object) {
			list($type, $data) = $this->repository->getRawObject($object);
			$blob = Git::getTypeName($type).' '.strlen($data)."\0".$data;
			$blob = gzcompress($blob);
			$blob = sha1_hex($object).' '.strlen($blob)."\0".$blob;
			$pack = $pack.$blob;
		}
		//return $pack;
		return base64_encode($pack);
	}
}

?>
