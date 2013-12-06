<?php

include_once 'Session.php';
include_once 'XMLProtocol.php';

set_include_path('phpseclib0.3.5');
include_once 'Crypt/RSA.php';

class AuthConst {
	static public $kAuthStanza = "auth";
	static public $kAuthSignedStanza = "auth_signed"; 
}


class AccountAuthStanzaHandler extends InStanzaHandler {
	private $inStreamReader;

	private $authType;
	private $userName;
	private $serverUser;

	public function __construct($inStreamReader) {
		InStanzaHandler::__construct(AuthConst::$kAuthStanza);
		$this->inStreamReader = $inStreamReader;
	}

	public function handleStanza($xml) {
		$this->authType = $xml->getAttribute("type");
		$this->userName = $xml->getAttribute("user");
		$this->serverUser = $xml->getAttribute("server_user");
		if ($this->authType == "" || $this->userName == "" || $this->userName == "")
			return false;
		return true;
	}

	public function finished() {
		if (strcmp($this->authType, "signature"))
			$this->inStreamReader->appendResponse(IqErrorOutStanza::makeErrorMessage("Unsupported authentication type."));

		// produce output
		$outStream = new ProtocolOutStream();
		$outStream->pushStanza(new IqOutStanza(IqType::$kResult));

		$signToken = "rand".rand()."time".time();
		$stanza = new OutStanza(AuthConst::$kAuthStanza);
		$stanza->addAttribute("sign_token", $signToken);
		$outStream->pushChildStanza($stanza);

		$this->inStreamReader->appendResponse($outStream->flush());
		Session::get()->setSignatureToken($signToken);
		Session::get()->setLoginUser($this->userName);
		Session::get()->setServerUser($this->serverUser);
	}
}


class AccountAuthSignedStanzaHandler extends InStanzaHandler {
	private $inStreamReader;

	private $signature;

	public function __construct($inStreamReader) {
		InStanzaHandler::__construct(AuthConst::$kAuthSignedStanza);
		$this->inStreamReader = $inStreamReader;
	}

	public function handleStanza($xml) {
		$this->signature = $xml->getAttribute("signature");
		if ($this->signature == "")
			return false;
			
		$this->signature = url_decode($this->signature);
		return true;
	}

	public function finished() {
		$signatureFileName = Session::get()->getServerUser()."/signature.pup";
		$publickey = "";
		if (file_exists($signatureFileName))
			$publickey = file_get_contents($signatureFileName);

		$rsa = new Crypt_RSA();
		$rsa->setSignatureMode(CRYPT_RSA_SIGNATURE_PKCS1);
		$rsa->setHash('md5');

		$roles = array();
		$rsa->loadKey($publickey);
		if ($rsa->verify(Session::get()->getSignatureToken(), $this->signature)) {
			Session::get()->setUserLoggedIn(true);
			$roles[] =  "account";
		} else { 
			// TODO add proper login
			Session::get()->setUserLoggedIn(true);
			$roles[] =  "post_message";
		}
		Session::get()->setUserRoles($roles);
	
		// produce output
		$outStream = new ProtocolOutStream();
		$outStream->pushStanza(new IqOutStanza(IqType::$kResult));
		$stanza = new OutStanza(AuthConst::$kAuthSignedStanza);
		$outStream->pushChildStanza($stanza);
		$roles = Session::get()->getUserRoles();
		$firstRole = true;
		foreach ($roles as $role) {
			$stanza = new OutStanza("role");
			$stanza->setText($role);
			if ($firstRole) {
				$outStream->pushChildStanza($stanza);
				$firstRole = false;
			} else
				$outStream->pushStanza($stanza);
		}
		$outStream->cdDotDot();
		$this->inStreamReader->appendResponse($outStream->flush());
	}
}


class LogoutStanzaHandler extends InStanzaHandler {
	private $inStreamReader;

	public function __construct($inStreamReader) {
		InStanzaHandler::__construct("logout");
		$this->inStreamReader = $inStreamReader;
	}

	public function handleStanza($xml) {
		Session::get()->clear();

		// produce output
		$outStream = new ProtocolOutStream();
		$outStream->pushStanza(new IqOutStanza(IqType::$kResult));
		$stanza = new OutStanza("logout");
		$stanza->addAttribute("status", "ok");
		$outStream->pushChildStanza($stanza);
		$this->inStreamReader->appendResponse($outStream->flush());

		return true;
	}
}
?>
