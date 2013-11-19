<?php

class Session {
	private function __construct() {
	}
    
	public static function get() {
		static $sSession = null;
		if ($sSession === NULL)
			$sSession = new Session();
		return $sSession;
	}
	
	public function setSignatureToken($token) {
		$_SESSION['sign_token'] = $token;
	}
	
	public function getSignatureToken() {
		return $_SESSION['sign_token'];
	}

	public function setUserName($user) {
		$_SESSION['user'] = $user;
	}
	
	public function getUserName() {
		return $_SESSION['user'];
	}

	public function setUserLoggedIn($bool) {
		$_SESSION['user_auth'] = $bool;
	}
	
	public function isUserLoggedIn() {
		if (!isset($_SESSION['user_auth']))
			return false;
		return $_SESSION['user_auth'];
	}

	public function getUserDir() {
		return $_SESSION['user'];
	}
}

?>
