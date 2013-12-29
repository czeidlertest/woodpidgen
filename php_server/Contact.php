<?php

include_once 'UserData.php';


class Contact extends UserData {
	private $keys = null;
	private $storageKeyId;

	private $uid;

	public function __construct($userData, $directory, $uid) {
		parent::__construct($userData, $directory)

		$this->uid = $uid;
		$this->write("uid", $this->uid);
	}

	public function getUid() {
		return $this->uid;
	}

	public function getKeySet($keyId, &$certificate, &$publicKey) {
		$this->read("keys/".keyId."/certificate", $certificate);
		$this->read("keys/".keyId."/publicKey", $certificate);
	}

	public function addKeySet($keyId, $certificate, $publicKey) {
		$this->write("keys/".keyId."/certificate", $certificate);
		$this->write("keys/".keyId."/publicKey", $certificate);
	}

	public function setMainKeyId($mainKeyId) {
		$this->write("mainKeyId", $mainKeyId);
	}

	public function getMainKeyId() {
		$mainKeyId;
		$this->read("mainKeyId", $mainKeyId);
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
