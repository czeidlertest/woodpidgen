<?php

include_once 'UserData.php';
include_once 'UserIdentity.php';


class Profile extends UserData {
	public function __construct($database, $branch, $directory) {
		parent::__construct($database, $branch, $directory);
	}

	public function getUserIdentityAt($index) {
		$ids = listDirectories("user_identities");
		$identityId = $ids[$index];
		$branch;
		$baseDir;
		read("user_identities/".$identityId."/database_branch", $branch);
		read("user_identities/".$identityId."/database_base_dir", $baseDir);

		$identity = new UserIdentity($database, $branch, $baseDir);
		$identity->open();
		return $identity;
	}
}

?>
