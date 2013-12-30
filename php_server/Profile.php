<?php

include_once 'KeyStore.php';
include_once 'UserData.php';
include_once 'UserIdentity.php';


class Profile extends UserData {
	public function __construct($database, $branch, $directory) {
		parent::__construct($database, $branch, $directory);
	}

	public function getUserIdentityAt($index) {
		$ids = $this->listDirectories("user_identities");
		$identityId = $ids[$index];
		$branch;
		$baseDir;
		read("user_identities/".$identityId."/database_branch", $branch);
		read("user_identities/".$identityId."/database_base_dir", $baseDir);

		$identity = new UserIdentity($database, $branch, $baseDir);
		$identity->open();
		return $identity;
	}

	public function getUserIdentityKeyStore($userIdentity) {
		$keyStoreId = $userIdentity->getKeyStoreId();
		$keyStoreIds = $this->listDirectories("key_stores");
		if (!in_array($keyStoreId, $keyStoreIds)
			return null;
		$branch;
		$baseDir;
		read("key_stores/".$keyStoreId."/database_branch", $branch);
		read("key_stores/".$keyStoreId."/database_base_dir", $baseDir);
		$keyStore = new KeyStore($database, $branch, $baseDir);
		return $keyStore;
	}
}

?>
