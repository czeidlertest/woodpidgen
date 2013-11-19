<?php

include_once 'Session.php';
include_once 'XMLProtocol.php';

set_include_path('phpseclib0.3.5');
include_once 'Crypt/RSA.php';


class UserAuthStanzaHandler extends InStanzaHandler {
	private $inStreamReader;

	private $authType;
	private $userName;

	public function __construct($inStreamReader) {
		InStanzaHandler::__construct("user_auth");
		$this->inStreamReader = $inStreamReader;
	}

	public function handleStanza($xml) {
		$this->authType = $xml->getAttribute("type");
		$this->userName = $xml->getAttribute("user");
		if ($this->authType == "" || $this->userName == "")
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
		$stanza = new OutStanza("user_auth");
		$stanza->addAttribute("sign_token", $signToken);
		$outStream->pushChildStanza($stanza);

		$this->inStreamReader->appendResponse($outStream->flush());
		Session::get()->setSignatureToken($signToken);
		Session::get()->setUserName($this->userName);
	}
}


class UserAuthSignedStanzaHandler extends InStanzaHandler {
	private $inStreamReader;
	
	private $signature;

	public function __construct($inStreamReader) {
		InStanzaHandler::__construct("user_auth_signed");
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
		$publickey = file_get_contents(Session::get()->getUserName()."/signature.pup");

		$rsa = new Crypt_RSA();
		$rsa->setSignatureMode(CRYPT_RSA_SIGNATURE_PKCS1);
		$rsa->setHash('md5');
		
		$rsa->loadKey($publickey);
		$verification = "denied";
		if ($rsa->verify(Session::get()->getSignatureToken(), $this->signature)) {
			$verification = "ok";
			Session::get()->setUserLoggedIn(true);
		}
	
		// produce output
		$outStream = new ProtocolOutStream();
		$outStream->pushStanza(new IqOutStanza(IqType::$kResult));
		$stanza = new OutStanza("user_auth_signed");
		$stanza->addAttribute("verification", $verification);
		$outStream->pushChildStanza($stanza);
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
		Session::get()->setSignatureToken("");
		Session::get()->setUserName("");
		Session::get()->setUserLoggedIn(false);

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
