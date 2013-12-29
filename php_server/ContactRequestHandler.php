<?php

include_once 'XMLProtocol.php';
include_once 'Signature.php';

class ContactMessageConst {
	static public $kContactRequestStanza = "conctact_request";
};


class CertificateStanzaHandler extends InStanzaHandler {
	private $certificate;
	
	public function __construct() {
		InStanzaHandler::__construct("certificate");
	}
	
	public function handleStanza($xml) {
		$this->certificate = $xml->readString();
		if ($this->certificate == "")
			return false;
		return true;
	}

	public function getCertificate() {
		return $this->certificate;
	}
};


class PublicKeyStanzaHandler extends InStanzaHandler {
	private $publicKey;
	
	public function __construct() {
		InStanzaHandler::__construct("publicKey");
	}
	
	public function handleStanza($xml) {
		$this->publicKey = $xml->readString();
		if ($this->publicKey == "")
			return false;
		return true;
	}

	public function getCertificate() {
		return $this->publicKey;
	}
};


class ContactRequestStanzaHandler extends InStanzaHandler {
	private $serverUser;
	private $uid;
    private $keyId;

    private $certificateStanzaHandler;
	private $publicKeyStanzaHandler;

	public function __construct() {
		InStanzaHandler::__construct(ContactMessageConst::$kContactRequestStanza);

		$this->certificateStanzaHandler = new CertificateStanzaHandler();
		$this->addChild($this->certificateStanzaHandler);
		$this->publicKeyStanzaHandler = new PublicKeyStanzaHandler();
		$this->addChild($this->publicKeyStanzaHandler);
	}
	
	public function handleStanza($xml) {
		$this->serverUser =  $xml->getAttribute("serverUser");
		$this->uid = $xml->getAttribute("uid");
		$this->keyId = $xml->getAttribute("keyId");
		if ($this->serverUser == "" || $this->uid == "" || $this->keyId == "")
			return false;
		return true;
	}

	public function finished() {
		if (Sessiong::get()->getServerUser() != $serverUser) {
			Session::get()->clear();
			Session::get()->setServerUser($serverUser);
		}
		$publicKey = publicKeyStanzaHandler->getPublicKeyStanzaHandler();
		$certificate = certificateStanzaHandler->getCertificateStanzaHandler();

		$contact = new Contact($this, "contacts", $this->uid);
		$contact->addKeySet($this->keyId, $certificate, $publicKey)
		$contact->setMainKeyId($this->keyId);
		
		$profile = Session::get()->getProfile();
		$userIdentity = $profile->getUserIdentityAt(0);
		$userIdentity->addContact($contact);
		$userIdentity->commit();

		// reply
		$outStream = new ProtocolOutStream();
		$outStream->pushStanza(new IqOutStanza(IqType::$kResult));
		$stanza = new OutStanza(ContactMessageConst::$kContactRequestStanza);
		$stanza->addAttribute("status", "ok");

		//TODO get own public keys 
		$stanza->addAttribute("uid", $userIdentity->getUid());
		$stanza->addAttribute("keyId", $myKeyId);
    
		$outStream->pushChildStanza($stanza);
		
		$certificateStanza = new OutStanza("certificate");
		$certificateStanza->setText($myCertificate);
		$outStream->pushChildStanza($certificateStanza);
		$outStream->cdDotDot();
		
		$publicKeyStanza = new OutStanza("publicKey");
		$publicKeyStanza->setText($myPublicKey);
		$outStream->pushChildStanza($publicKeyStanza);
		$outStream->cdDotDot();

		$this->inStreamReader->appendResponse($outStream->flush());
	}
	
	public function printError($error, $message) {
		// produce output
		$outStream = new ProtocolOutStream();
		$outStream->pushStanza(new IqOutStanza(IqType::$kResult));
		$stanza = new OutStanza(ContactMessageConst::$kContactRequestStanza);
		$stanza->addAttribute("status", $error);
		$stanza->setText($message);
		$outStream->pushChildStanza($stanza);
		$this->inStreamReader->appendResponse($outStream->flush());
	}
}

?> 
