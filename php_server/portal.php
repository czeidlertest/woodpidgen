<?php

session_start();

require 'Crypt/DiffieHellman.php';
require 'RetrieveMessages.php'; 
require 'phpseclib0.3.5/Crypt/AES.php';


interface IPortalInterface
{
    public function receiveData($data);
    public function sendData($data);
}

class PlainTextPortal implements IPortalInterface{
	public function receiveData($data)
	{
		return $data;
	}

	public function sendData($data) {
		echo $data;
    }
};


class EncryptedPortal implements IPortalInterface{
	private $fKey;
	private $fIV;

	public function __construct($key, $iv) {
		$this->fAES = new Crypt_AES(CRYPT_AES_MODE_CBC);
		$this->fAES->setKeyLength(128);
		$this->fAES->setKey($key);
		$this->fAES->setIV($iv);
		$this->fKey = $key;
		$this->fIV = $iv;
	}

	public function receiveData($data)
	{
		$data = str_replace(" ", "+", $data);
		$this->fAES->setKey($this->fKey);
		$this->fAES->setIV($this->fIV);
		return $this->fAES->decrypt(base64_decode($data));
	}

    public function sendData($data) {
		$this->fAES->setKey($this->fKey);
		$this->fAES->setIV($this->fIV);
		return base64_encode($this->fAES->encrypt($data));
    }
    
};


$gPortal = new PlainTextPortal();
$gOutput = "";

function writeToOutput($string) {
	global $gOutput;
	$gOutput = $gOutput.$string;
}

function flushResponce() {
	global $gPortal, $gOutput;
	echo $gPortal->sendData($gOutput);
}

function finished() {
	flushResponce();
	exit(0);
}


if (empty($_POST['request']))
	die("invalid request");

$request = $_POST['request'];
if ($request == "neqotiate_dh_key") {
	if (empty($_POST['dh_prime']) || empty($_POST['dh_base']) || empty($_POST['dh_public_key'])) {
		echo XMLResponse::error(-1, "missing values for Diffie Hellman negotiation"); 
		die();
	}
	$math = new Crypt_DiffieHellman_Math('gmp');
	$randomNumber = $math->rand(2, '384834813984910010746469093412498181642341794');

	$dh = new Crypt_DiffieHellman($_POST['dh_prime'], $_POST['dh_base'], $randomNumber);
	$dh->generateKeys();
	
	$dh->computeSecretKey($_POST['dh_public_key']);
	$sharedKey = unpack('C*', $dh->getSharedSecretKey(Crypt_DiffieHellman::BINARY));
	// pad key to 128 byte
	for ($i = count($sharedKey); $i < 128; $i = $i + 1)
		array_push($sharedKey, '\0');

	$_SESSION['dh_private_key'] = base64_encode(call_user_func_array("pack", array_merge(array("C*"), $sharedKey)));
	
	writeToOutput(XMLResponse::diffieHellmanPublicKey($_POST['dh_prime'], $_POST['dh_base'], $dh->getPublicKey()));
	writeToOutput("public remote: ".$_POST['dh_public_key']."\n");
	writeToOutput("secrete: ".$randomNumber."\n");
	writeToOutput("secrete share: ".$dh->getSharedSecretKey()."\n");
	writeToOutput("binary secrete share: ".base64_encode($_SESSION['dh_private_key'])."\n");
	writeToOutput("iv share: ".$_POST['encrypt_iv']."\n");

	if (!empty($_POST['encrypt_iv']))
		$_SESSION['encrypt_iv'] = str_replace(" ", "+", $_POST['encrypt_iv']);

	finished();
}


// check if we use an encrypted connection and set portal accordantly
if (isset($_SESSION['dh_private_key']) && isset($_SESSION['encrypt_iv']))
	$gPortal = new EncryptedPortal(base64_decode($_SESSION['dh_private_key']), base64_decode($_SESSION['encrypt_iv']));
// TODO enable again
//else {
//	writeToOutput("php encryption required");
//	finished();
//}

// get data
$request = $gPortal->receiveData($request);


$XMLHandler = new XMLHandler($request);
$response = $XMLHandler->handle();

// start working
writeToOutput($response);
finished();

?>
