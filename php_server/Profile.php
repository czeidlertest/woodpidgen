<?php

include_once 'KeyStore.php';
include_once 'UserData.php';
include_once 'UserIdentity.php';


class Profile extends UserData {
	private $userIdentities = array();

	public function __construct($database, $branch, $directory) {
		parent::__construct($database, $branch, $directory);

		$ids = $this->listDirectories("user_identities");
		foreach ($ids as $identityId) {
			$branch;
			$baseDir;
			$this->read("user_identities/".$identityId."/database_branch", $branch);
			$this->read("user_identities/".$identityId."/database_base_dir", $baseDir);
			$identity = new UserIdentity($this->database, $branch, $baseDir);
			$identity->open();
			$this->userIdentities[] = $identity;
		}
	}

	public function getUserIdentityAt($index) {
		if ($index >= count($this->userIdentities))
			return null;
		return $this->userIdentities[$index];
	}

	public function getUserIdentityKeyStore($userIdentity) {
		$keyStoreId = $userIdentity->getKeyStoreId();
		$keyStoreIds = $this->listDirectories("key_stores");
		if (!in_array($keyStoreId, $keyStoreIds))
			return null;
		$branch;
		$baseDir;
		$this->read("key_stores/".$keyStoreId."/database_branch", $branch);
		$this->read("key_stores/".$keyStoreId."/database_base_dir", $baseDir);
		$keyStore = new KeyStore($this->getDatabase(), $branch, $baseDir);
		return $keyStore;
	}
}

?>
