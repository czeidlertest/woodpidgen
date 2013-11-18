<?php

include_once './XMLProtocol.php';


class UserAuthStanzaHandler extends InStanzaHandler {
	private $inStreamReader;
	private $database;

	private $authType;
	private $userName;

	public function __construct($inStreamReader, $database) {
		InStanzaHandler::__construct("user_auth");
		$this->inStreamReader = $inStreamReader;
		$this->database = $database;
	}

	public function handleStanza($xml) {
		$this->authType = $xml->getAttribute("type");
		$this->userName = $xml->getAttribute("user");
		return true;
	}

	public function finished() {
		if (strcmp($this->authType, "signature"))
			$this->inStreamReader->appendResponse(IqErrorOutStanza::makeErrorMessage("Unsupported authentication type."));

		// produce output
		$outStream = new ProtocolOutStream();
		$outStream->pushStanza(new IqOutStanza(IqType::$kResult));

		$signToken = "rand:".rand()."time:".time();
		$stanza = new OutStanza("user_auth");
		$stanza->addAttribute("sign_token", $signToken);
		$outStream->pushChildStanza($stanza);

		$this->inStreamReader->appendResponse($outStream->flush());
	}
}

?>
