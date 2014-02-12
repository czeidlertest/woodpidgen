<?php

include_once 'Session.php';
include_once 'UserData.php';


class SignedPackage {
	public $uid;
	public $sender;
	public $signatureKey;
	public $signature;
	public $data;
}

class Mailbox extends UserData {
	private $lastErrorMessage;

	public function __construct($database, $branch, $directory) {
		parent::__construct($database, $branch, $directory);

	}

	public function addChannel($messageChannel) {
		if (!$this->verifyPackage($messageChannel))
			return false;
		
		$path = $this->pathForChannelId($messageChannel->uid);
		return $this->writePackage($messageChannel, $path);
	}

	public function addMessage($channelId, $message) {
		if (!$this->isValid($channelId, $message))
			return false;

		$path = $this->pathForMessageId($channelId, $message->uid);
		return $this->writePackage($message, $path."/message");
	}

	public function getLastErrorMessage() {
		return $this->lastErrorMessage;
	}

	public function hasChannel($channelUId) {
		$path = $this->pathForChannelId($channelUId);
		$data;
		$result = $this->read($path, $data);
		if (!$result)
			return false;
		return true;
	}

	public function hasMessage($channelId, $messageUid) {
		$path = $this->pathForMessageId($channelId, $messageUid);
		$data;
		$ok = $this->read($path, $data);
		if (!$ok)
			return false;
		return true;
	}

	private function pathForMessageId($channelId, $messageId) {
		$channelPath = $this->dirForChannelId($channelId);
		$path = sprintf('%s/%s', substr($messageId, 0, 2), substr($messageId, 2));
		return $channelPath."/".$path;
	}

	private function pathForChannelId($channelId) {
		return $this->dirForChannelId($channelId)."/r";
	}
	
	private function dirForChannelId($channelId) {
		$path = sprintf('%s/%s', substr($channelId, 0, 2), substr($channelId, 2));
		return $path;
	}

	private function isValid($channelUid, $message) {
		if ($this->hasMessage($message->uid)) {
			$this->lastErrorMessage = "message with same uid exist";
			return false;
		}

		if (!$this->hasChannel($channelUid))
			return false;

		if (!$this->verifyPackage($message))
			return false;

		return true;
	}

	private function verifyPackage($package) {
		$signatureLength = unpack("N", $package->data)[1];
		$mainData = substr($package->data, $signatureLength + 4);
		$hash = hash('sha256', $mainData);

		$userIdentity = Session::get()->getMainUserIdentity();
		$sender = $userIdentity->findContact($package->sender);
		if ($sender === null) {
			$this->lastErrorMessage = "sender unknown";
			return false;
		}
	
		$ok = $sender->verify($package->signatureKey, $hash, $package->signature);
		if (!$ok)
			$this->lastErrorMessage = "bad signature";
		return $ok;
	}

	private function writePackage($package, $path) {
		return $this->write($path, $package->data);
	}
}

?>
