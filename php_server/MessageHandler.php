<?php

include_once 'XMLProtocol.php';


class MessageConst {
	static public $kMessageStanza = "message";
	static public $kChannelStanza = "channel";
	static public $kPrimaryDataStanza = "primary_data";
};


class ChannelLockCKeyStanzaHandler extends InStanzaHandler {
	public $ckey;

	public function __construct() {
		InStanzaHandler::__construct("ckey");
	}

	public function handleStanza($xml) {
		$this->ckey = $xml->readString();
		return true;
	}
};

class ChannelLockIVStanzaHandler extends InStanzaHandler {
	public $iv;

	public function __construct() {
		InStanzaHandler::__construct("iv");
	}

	public function handleStanza($xml) {
		$this->iv = $xml->readString();
		return true;
	}
};

class ChannelLockStanzaHandler extends InStanzaHandler {
	public $keyId;
	public $ivHandler;
	public $ckeyHandler;

	public function __construct() {
		InStanzaHandler::__construct("lock");
		$this->ivHandler = new ChannelLockIVStanzaHandler();
		$this->addChild($this->ivHandler, false);
		$this->ckeyHandler = new ChannelLockCKeyStanzaHandler();
		$this->addChild($this->ckeyHandler, false);
	}

	public function handleStanza($xml) {
		$this->keyId = $xml->getAttribute("key_id");
		if ($this->keyId == "")
			return false;
		return true;
	}
};

class ChannelStanzaHandler extends InStanzaHandler {
	public $uid;
	public $channelLockStanzaHandler;

	public function __construct() {
		InStanzaHandler::__construct(MessageConst::$kChannelStanza);
		$this->channelLockStanzaHandler = new ChannelLockStanzaHandler();
		$this->addChild($this->channelLockStanzaHandler, false);
	}
	
	public function handleStanza($xml) {
		$this->uid = $xml->getAttribute("uid");
		if ($this->uid == "")
			return false;
		return true;
	}
};


class MessageDataStanzaHandler extends InStanzaHandler {
	public $signatureKey;
	public $signature;
	public $data;

	public function __construct() {
		InStanzaHandler::__construct(MessageConst::$kPrimaryDataStanza);
	}
	
	public function handleStanza($xml) {
		$this->signatureKey = $xml->getAttribute("signature_key");
		$this->signature = $xml->getAttribute("signature");
		if ($this->signatureKey == "" || $this->signature == "")
			return false;
		$this->data = $xml->readString();
		return true;
	}
};


class MessageStanzaHandler extends InStanzaHandler {
	private $inStreamReader;
	
	public $channelUid;
	public $from;
	public $dataHandler;
	private $channelStanzaHandler;
	
	public function __construct($inStreamReader) {
		InStanzaHandler::__construct(MessageConst::$kMessageStanza);
		$this->inStreamReader = $inStreamReader;

		$this->channelStanzaHandler = new ChannelStanzaHandler();
		$this->dataHandler = new MessageDataStanzaHandler();

		$this->addChild($this->channelStanzaHandler, true);
		$this->addChild($this->dataHandler, false);
	}

	public function handleStanza($xml) {
		$this->channelUid = $xml->getAttribute("channel_uid");
		$this->from = $xml->getAttribute("from");
		if ($this->channelUid == "" || $this->from == "")
			return false;
		return true;
	}

	public function finished() {
		$database = Session::get()->getDatabase();
		if ($database === null)
			throw new exception("unable to get database");

		$header = $this->headerStanzaHandler->getText();
		$body = $this->bodyStanzaHandler->getText();

		$branch = "";
		$baseDir = "";
		$this->findMailbox($database, $branch, $baseDir);
		if ($branch == "")
			throw new exception("unable to find mailbox");

		$rootTree = clone $database->getRootTree($branch);

		# get message name
		$hash = hash_init('sha1');
		hash_update($hash, $header);
		hash_update($hash, $body);

		//foreach ($attachments as $attachment)
		//	hash_update($hash, $attachment);
		$messageName = sha1_hex(hash_final($hash, TRUE));
		$path = "";
		if ($baseDir != "")
			$path = $baseDir."/";			
		$path = $path.$this->pathForMessageId($messageName);

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
		
		// produce output
		$outStream = new ProtocolOutStream();
		$outStream->pushStanza(new IqOutStanza(IqType::$kResult));

		$stanza = new OutStanza(MessageConst::$kMessageStanza);
		$stanza->addAttribute("status", "message_received");
		$outStream->pushChildStanza($stanza);

		$this->inStreamReader->appendResponse($outStream->flush());
	}

	private function findMailbox($database, &$branch, &$dir) {
		$branch = "";
		$dir = "";
		$maiboxUid = $database->readBlobContent("profile", "main_mailbox");
		if ($maiboxUid === null)
			return;
		$branch = $database->readBlobContent("profile", "mailboxes/".$maiboxUid."/database_branch");
		$dir = $database->readBlobContent("profile", "mailboxes/".$maiboxUid."/database_base_dir");
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
