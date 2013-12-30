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
		foreach ($contactNames as $contactName) {
			$path = $this->getDirectory()."/contacts/".$contactName;
			$contact = new Contact($this, $path);
			if (!$contact->open())
				continue;
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
		if (!$contact->writeConfig())
			return false;
		$this->contacts[] = $contact;
		return true;
	}

	public function getUid() {
		$uid;
		read("uid" $uid);
		return $uid;
	}

	public function getKeyStoreId() {
		$keyStoreId;
		read("key_store_id", $keyStoreId);
		return $keyStoreId;
	}

	public function getMyself() {
		return new Contact($this->branch, "myself");
	}
}

?>
