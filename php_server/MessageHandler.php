<?php

include_once 'Mailbox.php';
include_once 'XMLProtocol.php';


class MessageConst {
	static public $kMessageStanza = "message";
	static public $kChannelStanza = "channel";
	static public $kPrimaryDataStanza = "primary_data";
};


class ChannelLockCKeyStanzaHandler extends InStanzaHandler {
	private $messageChannel;

	public function __construct($messageChannel) {
		InStanzaHandler::__construct("ckey");
		$this->messageChannel = $messageChannel;
	}

	public function handleStanza($xml) {
		$this->messageChannel->setCKey($xml->readString());
		return true;
	}
};

class ChannelLockIVStanzaHandler extends InStanzaHandler {
	private $messageChannel;

	public function __construct($messageChannel) {
		InStanzaHandler::__construct("iv");
		$this->messageChannel = $messageChannel;
	}

	public function handleStanza($xml) {
		$this->messageChannel->setIV($xml->readString());
		return true;
	}
};

class ChannelLockStanzaHandler extends InStanzaHandler {
	private $messageChannel;
	public $ckeyHandler;

	public function __construct($messageChannel) {
		InStanzaHandler::__construct("lock");
		$this->messageChannel = $messageChannel;
		$this->ivHandler = new ChannelLockIVStanzaHandler($this->messageChannel);
		$this->addChild($this->ivHandler, false);
		$this->ckeyHandler = new ChannelLockCKeyStanzaHandler($this->messageChannel);
		$this->addChild($this->ckeyHandler, false);
	}

	public function handleStanza($xml) {
		$this->messageChannel->setAsymKeyId($xml->getAttribute("key_id"));
		if ($this->messageChannel->getAsymKeyId() == "")
			return false;
		return true;
	}
};

class ChannelStanzaHandler extends InStanzaHandler {
	private $messageChannel;
	private $channelLockStanzaHandler;

	public function __construct($messageChannel) {
		InStanzaHandler::__construct(MessageConst::$kChannelStanza);
		$this->messageChannel = $messageChannel;
		$this->channelLockStanzaHandler = new ChannelLockStanzaHandler($this->messageChannel);
		$this->addChild($this->channelLockStanzaHandler, false);
	}
	
	public function handleStanza($xml) {
		$this->messageChannel->setUid($xml->getAttribute("uid"));
		if ($this->messageChannel->getUid() == "")
			return false;
		return true;
	}
};


class MessageDataStanzaHandler extends InStanzaHandler {
	private $message;

	public function __construct($message) {
		InStanzaHandler::__construct(MessageConst::$kPrimaryDataStanza);
		$this->message = $message;
	}
	
	public function handleStanza($xml) {
		$this->message->setSignatureKey($xml->getAttribute("signature_key"));
		$this->message->setSignature($xml->getAttribute("signature"));
		if ($this->message->getSignatureKey() == "" || $this->message->getSignature() == "")
			return false;
		$this->message->setData($xml->readString());
		return true;
	}
};


class MessageStanzaHandler extends InStanzaHandler {
	private $inStreamReader;

	private $messageChannel;
	private $message;
	private $dataHandler;
	private $channelStanzaHandler;
	
	public function __construct($inStreamReader) {
		InStanzaHandler::__construct(MessageConst::$kMessageStanza);
		$this->inStreamReader = $inStreamReader;

		$this->messageChannel = new MessageChannel();
		$this->message = new Message();

		$this->channelStanzaHandler = new ChannelStanzaHandler($this->messageChannel);
		$this->dataHandler = new MessageDataStanzaHandler($this->message);

		$this->addChild($this->channelStanzaHandler, true);
		$this->addChild($this->dataHandler, false);
	}

	public function handleStanza($xml) {
		$this->message->setChannelUid($xml->getAttribute("channel_uid"));
		$this->message->setFrom($xml->getAttribute("from"));
		if ($this->message->getChannelUid() == "" || $this->message->getFrom() == "")
			return false;
		return true;
	}

	public function finished() {
		$profile = Session::get()->getProfile();
		if ($profile === null)
			throw new exception("unable to get profile");

		$mailbox = $profile->getMainMailbox($message);
		if ($mailbox === null)
			throw new exception("unable to get mailbox");

		$ok = $mailbox->addMessage($this->messageChannel, $this->message);

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
