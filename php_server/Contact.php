<?php

include_once 'UserData.php';


class Contact extends UserData {
	private $keys = null;
	private $storageKeyId;

	private $uid;

	public function __construct($userData, $directory) {
		parent::__construct($userData->getDatabase(), $userData->getBranch(), $directory);
	}

	public function setUid($uid) {
		$this->uid = $uid;
		$this->write("uid", $this->uid);
	}

	public function getUid() {
		$this->read("uid", $this->uid);
		return $this->uid;
	}

	public function getKeySet($keyId, &$certificate, &$publicKey) {
		$this->read("keys/".$keyId."/certificate", $certificate);
		$this->read("keys/".$keyId."/public_key", $certificate);
	}

	public function addKeySet($keyId, $certificate, $publicKey) {
		$this->write("keys/".$keyId."/certificate", $certificate);
		$this->write("keys/".$keyId."/public_key", $certificate);
	}

	public function setMainKeyId($mainKeyId) {
		$this->write("keys/main_key_id", $mainKeyId);
	}

	public function getMainKeyId() {
		$mainKeyId;
		$this->read("keys/main_key_id", $mainKeyId);
		return $mainKeyId;
	}

	public function addData($path, $data) {
		$this->write($path, $data);
	}

	public function open() {
		$type;
		$this->read("keystore_type", $type);
		if (type == "private")
			return false;
		$this->read("uid", $this->uid);
		return true;
	}
}

?>
