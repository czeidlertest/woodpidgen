<?php

include 'database.php'; 


$repoPath = 'test.git';

$messenger = new MessageFeeder($repoPath);
$messenger->init();
$attachments = array();
$messenger->addMessage("header".rand(0,100), "Hallo World".rand(0,100), $attachments);

/*
$repo = new Git($repoPath);

$object = new GitBlob($repo);

$object->data = "test string";

$object->rehash();
$object->write();


$name = $object->getName();

$obj = $repo->getObject($name);
echo $obj->data;
echo "<br>";

echo "Tree test:<br>";
$tree = new GitTree($repo);
$tree->updateNode("data", 100644, $name);
$tree->updateNode("testFile", 100644, $name);

$tree->rehash();
$tree->write();

$name = $tree->getName();
$obj = $repo->getObject($name);

$treeArray = $tree->listRecursive();

foreach ($treeArray as $key => $value)
{
	echo "$key = ".sha1_hex($repo->getObject($value)->getName())."<br>";
}
*/


class MessageFeeder {
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


?>
