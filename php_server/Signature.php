<?php

set_include_path('phpseclib0.3.5');
include_once 'Crypt/RSA.php';
 
class Signature {
	public $rsa;

	public function __construct($publickey) {
		$this->rsa = new Crypt_RSA();
		$this->rsa->setSignatureMode(CRYPT_RSA_SIGNATURE_PKCS1);
		$this->rsa->setHash('md5');

		$this->rsa->loadKey($publickey);
	}

	public function verify($data, $signature) {
		return $this->rsa->verify($data, $siganture);
	}
}
