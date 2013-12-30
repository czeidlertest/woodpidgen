<?php

include_once 'Session.php';
include_once 'Signature.php';
include_once 'XMLProtocol.php';


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

		$signToken = "rand".rand()."time".time();
		Session::get()->setSignatureToken($signToken);
		Session::get()->setLoginUser($this->userName);
		Session::get()->setServerUser($this->serverUser);

		// Check if the user has a change to login, i.e., if we know him
		$userIdentity = Session::get()->getMainUserIdentity();
		// if $userIdentity is null the database is invalid give the user a change to upload a profile
		if ($userIdentity != null) {
			$contact = $userIdentity->findContact($this->userName);
			if ($userIdentity->getMyself()->getUid() != $this->userName && $contact === null) {
				Session::get()->clear();
				// produce output
				$outStream = new ProtocolOutStream();
				$outStream->pushStanza(new IqOutStanza(IqType::$kResult));
				$stanza = new OutStanza(AuthConst::$kAuthStanza);
				$stanza->addAttribute("status", "i_dont_know_you");
				$outStream->pushChildStanza($stanza);

				$this->inStreamReader->appendResponse($outStream->flush());
				return;
			}
		}

		
		// produce output
		$outStream = new ProtocolOutStream();
		$outStream->pushStanza(new IqOutStanza(IqType::$kResult));
		$stanza = new OutStanza(AuthConst::$kAuthStanza);
		$stanza->addAttribute("status", "sign_this_token");
		$stanza->addAttribute("sign_token", $signToken);
		$outStream->pushChildStanza($stanza);

		$this->inStreamReader->appendResponse($outStream->flush());
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
		$userIdentity = Session::get()->getMainUserIdentity();

		$roles = array();
		if ($userIdentity != null) {
			$loginUser = Session::get()->getLoginUser();
			$myself = $userIdentity->getMyself();
			if ($myself->getUid() == $loginUser) {
				$this->accountLogin($myself, $roles);
			} else {
				$contact = $userIdentity->findContact($loginUser);
				if ($contact !== null)
					$this->userLogin($contact, $roles);
			}
		} else {
			$this->setupLogin($roles);
		}
		Session::get()->setUserRoles($roles);

		// produce output
		$outStream = new ProtocolOutStream();
		$outStream->pushStanza(new IqOutStanza(IqType::$kResult));
		$stanza = new OutStanza(AuthConst::$kAuthSignedStanza);
		$status;
		if (count($roles) > 0)
			$status = "ok";
		else
			$status = "denied";
		$stanza->addAttribute("status", $status);
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

	private function setupLogin(&$roles) {
		$signatureFileName = Session::get()->getServerUser()."/signature.pup";
		$publickey = "";
		if (file_exists($signatureFileName))
			$publickey = file_get_contents($signatureFileName);

		$signatureVerifier = new SignatureVerifier($publickey);
		if (!$signatureVerifier->verify(Session::get()->getSignatureToken(), $this->signature))
			return;
		$roles[] =  "account";
	}

	private function accountLogin($contact, &$roles) {
		if (!$this->verifyContactData($contact, Session::get()->getSignatureToken(), $this->signature))
			return;
		$roles[] =  "account";
	}

	private function userLogin($contact, &$roles) {
		if (!$this->verifyContactData($contact, Session::get()->getSignatureToken(), $this->signature))
			return;
		$roles[] = "contact_user";
	}

	private function verifyContactData($contact, $data, $signature) {
		$mainKeyId = $contact->getMainKeyId();
		$certificate;
		$publicKey;
		$contact->getKeySet($mainKeyId, $certificate, $publicKey);

		$signatureVerifier = new SignatureVerifier($publicKey);
		return $signatureVerifier->verify($data, $signature);
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
