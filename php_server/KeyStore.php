<?PHP

include_once 'UserData.php';

class KeyStore extends UserData {
	public function __construct($database, $branch, $directory) {
		parent::__construct($database, $branch, $directory);
	}

	public function readAsymmetricKey($keyId, &$certificate, &$publicKey) {
		$this->read($keyId."/certificate", $certificate);
		$this->read($keyId."/public_key", $publicKey);
	}

}

?>
