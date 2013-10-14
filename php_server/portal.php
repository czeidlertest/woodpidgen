<?php

session_start();

require 'Crypt/DiffieHellman.php';
require 'RetrieveMessages.php'; 
require 'phpseclib0.3.5/Crypt/AES.php';

/*
$key = base64_decode("jh6y5VcEuDXpUTrLWq8pJYyMHmLQCBOGSGWh2YiC0GBr0VRzsBV79HdxQJ8t7utUniNr6LG4Av8TT7MSwQnIPA==");
$iv = base64_decode("qf90IciaKTsBk/yS4r27 vCRXgLWYrQCWq1vAz54DVw1JUfTYy45vMXmQMZmsGAVCi/dJqP6hE1PmkjHV9lOGdcut6Syd6Vc1y2eVT21Obpa4woS6ku9oTi6TVAP1bVDsKrblM/F63UJYXRc/7og5n kBx3N6kMYvc2L3clLbUwBWxctA SpeUDuhftEARn0YsNubw/tWObH5sSi1FEMzzyq0oaoPXmTeq9clE5TX6Q U7sBnJtk3 T4sJFaXAoLNVAJAAOupDD6fzcOOYHbpRCirkFmMRWLay9lSyDR/ORM lAMNDxkLnrgPfEcex5ioblmNj1o2TDw3yNItFQDr9dRd0SNWVAddc0OuekutZ/ZATeeuSHIBXuFve0K4NgWFGJmYsw0kHtIP28hQS8KhN71B3vBUIJfMknQXDLDIWsW9oMP1xrOOzrQ2hQZHgwIjgawmUfbA4246AM6cuXEWLWoF4vHf2g0HG1UJSdt5pGWeycUee82wmcKiYnalRxWCqaCXP0eE0bfBaWAkScuMJuZwscGCsVTy8Sl3MF5vKpqQ6M3frYgsTW1zZYsow9Yzkm1rZBCWh1VbBtO9TNP6ksU2/eOdQSWuPAQQo9qm6OB8IW5gDI3Ndz9OIZERt5/FNkB872qZNMtET2ddFWHVXGQhURR6nwR7TWjYk5k Ck=");

$aes = new Crypt_AES(CRYPT_AES_MODE_CBC);
$aes->setKey($key);
$aes->setIV($iv);
		
for ($i = count(key); $i < 256; $i = $i + 1)
	$key.'\0';

$testData = "Hello World";
$encrypted = $aes->encrypt($testData);
echo base64_encode($encrypted)."\n";

echo $aes->decrypt($encrypted)."\n";

    */

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
		//echo "remote1: ".strlen($key)." ".base64_encode($key)." iv ".base64_encode($iv)."\n";
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
		//echo "remote: ".strlen($this->fAES->key)." ".base64_encode($this->fAES->key)." iv ".base64_encode($this->fIV)."\n";
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

//echo "remote0: ".strlen($_SESSION['dh_private_key'])." ".$_SESSION['dh_private_key']." iv ".$_SESSION['encrypt_iv']."\n";
		
// check if we use an encrypted connection and set portal accordantly
if (isset($_SESSION['dh_private_key']) && isset($_SESSION['encrypt_iv']))
	$gPortal = new EncryptedPortal(base64_decode($_SESSION['dh_private_key']), base64_decode($_SESSION['encrypt_iv']));

// get data
//echo "input ".$request."\n";
$response = $gPortal->receiveData($request);
//echo "input2 ".$response."\n";
// start working
writeToOutput($response." pong");
finished();


?>
