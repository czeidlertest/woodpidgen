<?php

include_once 'XMLProtocol.php';


class MessageConst {
	static public $kMessageStanza = "message";
	static public $kHeaderStanza = "header";
	static public $kBodyStanza = "body";
};


class HeaderStanzaHandler extends InStanzaHandler {
	private $text;

	public function __construct() {
		InStanzaHandler::__construct(MessageConst::$kHeaderStanza);
	}
	
	public function handleStanza($xml) {
		$this->text = $xml->readString();
		return true;
	}

	public function getText() {
		return $this->text;
	}
};


class BodyStanzaHandler extends InStanzaHandler {
	private $text;

	public function __construct() {
		InStanzaHandler::__construct(MessageConst::$kBodyStanza);
	}
	
	public function handleStanza($xml) {
		$this->text = $xml->readString();
		return true;
	}

	public function getText() {
		return $this->text;
	}
};


class MessageStanzaHandler extends InStanzaHandler {
	private $inStreamReader;

	private $headerStanzaHandler;
	private $bodyStanzaHandler;

	public function __construct($inStreamReader) {
		InStanzaHandler::__construct(MessageConst::$kMessageStanza);
		$this->inStreamReader = $inStreamReader;

		$this->headerStanzaHandler = new HeaderStanzaHandler();
		$this->bodyStanzaHandler = new BodyStanzaHandler();
		
		$this->addChild($this->headerStanzaHandler);
		$this->addChild($this->bodyStanzaHandler);
	}

	public function handleStanza($xml) {
		return true;
	}

	public function finished() {

		$database = Session::get()->getDatabase();
		if ($database === null)
			throw new exception("unable to get database");

		$header = $this->headerStanzaHandler->getText();
		$body = $this->bodyStanzaHandler->getText();

		$branch = "messages";
		$rootTree = $database->getRootTree($branch);

		# get message name
		$hash = hash_init('sha1');
		hash_update($hash, $header);
		//hash_update($hash, $body);
		//foreach ($attachments as $attachment)
		//	hash_update($hash, $attachment);
		$messageName = sha1_hex(hash_final($hash, TRUE));
		$path = $this->pathForMessageId($messageName);

		$treeBuilder = new TreeBuilder($rootTree);
		# build new tree
		$headerObj = $database->writeBlob($header);
		$treeBuilder->updateFile("$path/header", $headerObj->getName());

		$bodyObj = $database->writeBlob($body);
		$treeBuilder->updateFile("$path/body", $bodyObj->getName());

/*		$attachmentsObj = array();
		foreach ($attachments as $attachment) {
			$attachmentObj = $database->writeBlob($attachment->data);
			$treeBuilder->updateNode(
				"messages/$messageName/".$attachment->name, 100644,
				$attachmentObj->getName());
		}
*/

		$treeBuilder->write();
		$rootTree->rehash();
		$rootTree->write();

		$this->commit($database, $rootTree->getName(), $branch);
	}

	private function commit($database, $tree, $branch) {
		# write commit
		$parents = array();
		try {
			$tip = $database->getTip($branch);
			if (strlen($tip) > 0)
				$parents[] = $tip;
		} catch (Exception $e) {
		} 

		$commit = new GitCommit($database);
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
		$database->setBranchTip($branch, sha1_hex($commit->getName()));

		return $commit;
	}

	private function pathForMessageId($messageId) {
		$path = sprintf('%s/%s', substr($messageId, 0, 2), substr($messageId, 2));
		return $path;
	}
}


?>
