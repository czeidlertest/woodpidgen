<?php

include_once 'Mailbox.php';
include_once 'XMLProtocol.php';


class MessageConst {
	static public $kPutMessageStanza = "put_message";
	static public $kMessageStanza = "message";
	static public $kChannelStanza = "channel";
};


class SignedPackageStanzaHandler extends InStanzaHandler {
	private $signedPackage;
	
	public function __construct($signedPackage, $stanzaName) {
		InStanzaHandler::__construct($stanzaName);
		$this->signedPackage = $signedPackage;
	}
	
	public function handleStanza($xml) {
		$this->signedPackage->uid = $xml->getAttribute("uid");
		$this->signedPackage->sender =$xml->getAttribute("sender");
		$this->signedPackage->signatureKey = $xml->getAttribute("signatureKey");
		$this->signedPackage->signature = base64_decode($xml->getAttribute("signature"));
		$this->signedPackage->data = base64_decode($xml->readString());
		return true;
	}
};


class MessageStanzaHandler extends InStanzaHandler {
	private $inStreamReader;

	private $messageChannel;
	private $message;
	private $messageStanzaHandler;
	private $channelStanzaHandler;
	
	public function __construct($inStreamReader) {
		InStanzaHandler::__construct(MessageConst::$kPutMessageStanza);
		$this->inStreamReader = $inStreamReader;

		$this->messageChannel = new SignedPackage();
		$this->message = new SignedPackage();

		$this->channelStanzaHandler = new SignedPackageStanzaHandler($this->messageChannel, MessageConst::$kChannelStanza);
		$this->messageStanzaHandler = new SignedPackageStanzaHandler($this->message, MessageConst::$kMessageStanza);

		$this->addChild($this->channelStanzaHandler, true);
		$this->addChild($this->messageStanzaHandler, false);
	}

	public function handleStanza($xml) {
		return true;
	}

	public function finished() {
		$profile = Session::get()->getProfile();
		if ($profile === null)
			throw new exception("unable to get profile");

		$mailbox = $profile->getMainMailbox();
		if ($mailbox === null)
			throw new exception("unable to get mailbox");

		$messageChannel = null;
		if ($this->channelStanzaHandler->hasBeenHandled())
			$messageChannel = $this->messageChannel;
		$ok = $mailbox->addMessage($messageChannel, $this->message);

		// produce output
		$outStream = new ProtocolOutStream();
		$outStream->pushStanza(new IqOutStanza(IqType::$kResult));

		$stanza = new OutStanza(MessageConst::$kMessageStanza);
		if ($ok)
			$stanza->addAttribute("status", "message_received");
		else {
			$stanza->addAttribute("status", "declined");
			$stanza->addAttribute("error", $mailbox->getLastErrorMessage());
		}
		$outStream->pushChildStanza($stanza);

		$this->inStreamReader->appendResponse($outStream->flush());
	}
}


?>
