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

	public function addMessage($messageChannel, $message) {
		if (!$this->isValid($messageChannel, $message))
			return false;

		if ($messageChannel != null) {
			$uid = $messageChannel->uid;
			$path = $this->pathForChannelId($uid);
			$ok = $this->writePackage($messageChannel, $path);
			if (!$ok)
				return false;
		}

		$path = $this->pathForMessageId($uid);
		$ok= $this->writePackage($message, $path."/message");
		if (!$ok)
			return false;

		return $this->commit() !== null;

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

	public function hasMessage($messageUid) {
		$path = $this->pathForMessageId($messageUid);
		$data;
		$ok = $this->read($path, $data);
		if (!$ok)
			return false;
		return true;
	}

	private function pathForMessageId($messageId) {
		$path = sprintf('%s/%s', substr($messageId, 0, 2), substr($messageId, 2));
		return $path;
	}

	private function pathForChannelId($messageId) {
		$path = sprintf('%s/%s', substr($messageId, 0, 2), substr($messageId, 2));
		return "channels/".$path;
	}

	private function isValid($messageChannel, $message) {
		if ($this->hasMessage($message->uid)) {
			$this->lastErrorMessage = "message with same uid exist";
			return false;
		}

		if (!$this->verifyPackage($messageChannel) || !$this->verifyPackage($message)) {
			return false;
		}

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
