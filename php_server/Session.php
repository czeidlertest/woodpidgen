<?php

include_once 'Profile.php';


class Session {
	private $database;

	private function __construct() {
		$this->database = null;
	}
    
	public static function get() {
		static $sSession = null;
		if ($sSession === NULL)
			$sSession = new Session();
		return $sSession;
	}

	public function clear() {
		$this->setSignatureToken("");
		$this->setLoginUser("");
		$this->setServerUser("");
		$this->setUserRoles(array());
		$this->setUserLoggedIn(false);
	}

	public function getDatabase() {
		if ($this->database !== null)
			return $this->database;
		$serverUser = $this->getServerUser();
		if ($serverUser == "")
			throw new Exception("no server user");
		return new GitDatabase($serverUser."/.git");
	}

	public function getProfile() {
		$database = $this->getDatabase();
		return new Profile($database, "profile", "");
	}

	public function getUserDir() {
		return $_SESSION['server_user'];
	}

	public function setSignatureToken($token) {
		$_SESSION['sign_token'] = $token;
	}

	public function getSignatureToken() {
		return $_SESSION['sign_token'];
	}

	public function setLoginUser($user) {
		$_SESSION['login_user'] = $user;
	}
	
	public function getLoginUser() {
		return $_SESSION['login_user'];
	}

	public function setServerUser($user) {
		$_SESSION['server_user'] = $user;
	}

	public function getServerUser() {
		if (!isset($_SESSION['server_user']))
			return "";
		return $_SESSION['server_user'];
	}

	public function setUserLoggedIn($bool) {
		$_SESSION['user_auth'] = $bool;
	}
	
	public function isUserLoggedIn() {
		if (!isset($_SESSION['user_auth']))
			return false;
		return $_SESSION['user_auth'];
	}

	public function setUserRoles($roles) {
		$_SESSION['user_roles'] = $roles;
	}
	
	public function getUserRoles() {
		if (!isset($_SESSION['user_roles']))
			return array();
		return $_SESSION['user_roles'];
	}
}

?>
