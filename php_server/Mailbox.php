<?php

include_once 'Session.php';
include_once 'UserData.php';


class MessageChannel {
	private $asymKeyId;
	private $iv;
	private $ckey;

	public function getAsymKeyId() {
		return $this->asymKeyId;
	}

	public function getIV() {
		return $this->iv;
	}

	public function getCKey() {
		return $this->ckey;
	}

	public function setAsymKeyId($asymKeyId) {
		$this->asymKeyId = $asymKeyId;
	}

	public function setIV($iv) {
		$this->iv = $iv;
	}

	public function setCKey($ckey) {
		$this->ckey = $ckey;
	}
};


class MessageData {
	private $signatureKey;
	private $signature;
	private $data;

	public function getSignatureKey() {
		return $this->signatureKey;
	}

	public function getSignature() {
		return $this->signature;
	}

	public function getData() {
		return $this->data;
	}

	public function setSignatureKey($signatureKey) {
		$this->signatureKey = $signatureKey;
	}

	public function setSignature($signature) {
		$this->signature = $signature;
	}

	public function setData($data) {
		$this->data = $data;
	}
}

class Message {
	private $channelUid;
	private $from;
	private $primaryPart;
	
	public function __construct() {
		$this->primaryPart = new MessageData();
	}

	public function getChannelUid() {
		return $this->channelUid;
	}

	public function getFrom() {
		return $this->from;
	}

	public function getPrimaryPart() {
		return $this->primaryPart;
	}

	public function setChannelUid($uid) {
		$this->channelUid = $uid;
	}

	public function setFrom($from) {
		$this->from = $from;
	}
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
			$data = $this->messageChannelToXML($messageChannel);

			$uid = $messageChannel->getUid();
			$path = $this->pathForChannelId($uid);
			$ok = $this->write($path, $data);
			if (!$ok)
				return false;
		}

		$data = messageToXML($message);
		$path = $this->pathForMessageId($uid);
		$ok= $this->write($path, $data);
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

	public function hasMessage($message) {
		$path = $this->pathForMessageId($channelUId);
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
		if ($messageChannel === null) {
			if (!$this->hasChannel($message->getChannelUid())) {
				$this->lastErrorMessage = "no such message channel";
				return false;
			}
		} else {
			if (!$this->hasChannel($messageChannel->getUid())) {
				$this->lastErrorMessage = "message channel exist";
				return false;
			}
			if ($messageChannel->getUid() != $message->getChannelUid()) {
				$this->lastErrorMessage = "invalid message channel";
				return false;
			}
		}
		
		if ($this->hasMessage($message->getUid())) {
			$this->lastErrorMessage = "message with same uid exist";
			return false;
		}

		// verify data signature
		$primaryPart = $message->getPrimaryPart();
		$userIdentity = Session::get()->getMainUserIdentity();
		$sender = $userIdentity->findContact($message->getFrom());
		$ok = $sender->verify($primaryPart->getSignatureKey(), $primaryPart->getData(), $primaryPart->getSignature());
		if (!$ok) {
			$this->lastErrorMessage = "bad signature";
			return false;
		}

		return true;
	}

	private function messageChannelToXML($messageChannel) {
		$outStream = new ProtocolOutStream();
		//OutStanze
		return $outStream->flush();
	}

	private function messageToXML($message) {
		$outStream = new ProtocolOutStream();
		// TODO
		return $outStream->flush();
	}
}

?>
