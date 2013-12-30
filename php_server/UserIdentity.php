<?php

include_once 'Contact.php';
include_once 'UserData.php';


/*! The server side can't encrypt or decrypt data in the database so just derive from UserData. */
class UserIdentity extends UserData {
	private $contacts = array();

	public function __construct($database, $branch, $directory) {
		parent::__construct($database, $branch, $directory);
	}

	public function open() {
		$contactNames = $this->listDirectories("contacts");
		if ($contactNames === null)
			return true;
		foreach ($contactNames as $contactName) {
			$path = $this->getDirectory()."/contacts/".$contactName;
			$contact = new Contact($this, $path);
			$this->contacts[] = $contact;
        }
        return true;
	}

	public function getContacts() {
		return $this->contacts;
	}

	public function addContact($contact) {
		$contactUid = $contact->getUid();
		$path = $this->getDirectory()."/contacts/".$contactUid;
		$contact->setDirectory($path);
		$this->contacts[] = $contact;
		return true;
	}
	
	public function findContact($uid) {
		foreach ($this->contacts as $contact) {
			if ($contact->getUid() == $uid)
				return $contact;
		}
		return null;
	}

	public function getUid() {
		$uid = "";
		$this->read("uid", $uid);
		return $uid;
	}

	public function getKeyStoreId() {
		$keyStoreId = "";
		$this->read("key_store_id", $keyStoreId);
		return $keyStoreId;
	}

	public function getMyself() {
		return new Contact($this, $this->getDirectory()."/myself");
	}
}

?>
