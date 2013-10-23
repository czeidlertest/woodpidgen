<?php
#if(rand(1,3) == 1){
    /* Fake an error */
 #   header("HTTP/1.0 404 Not Found");
  #  die();
#}

/* Send a string after a random number of seconds (2-10) */
#sleep(rand(0,1));

include 'database.php';


#$repoPath = 'test.git';
#$repository = new Git($repoPath);
#$syncManager = new MessageSyncManager($repository);

#$XMLHandler = new XMLHandler($_POST["request"], $syncManager);
#$XMLHandler->handle();


class XMLResponse {
	static public function qsGetQueryCommit($data, $branch, $first, $last) {
		$xml = new XMLWriter;
		$xml->openMemory();
		$xml->startDocument();
		$xml->startElement("iq");
		$xml->writeAttribute("type", "result");
		$xml->startElement("query");
		$xml->writeAttribute("xmlns", "git:transfer");
		$xml->startElement("pack");
		$xml->writeAttribute("branch", $branch);
		$xml->writeAttribute("first", $first);
		$xml->writeAttribute("last", $last);
		$xml->text($data);
		$xml->endElement();
		$xml->endElement();
		$xml->endElement();
		$xml->endDocument();

		return $xml->outputMemory();
	}

	static public function error($code, $message) {
		$xml = new XMLWriter;
		$xml->openMemory();
		$xml->startDocument();
		$xml->startElement("error");
		$xml->writeAttribute("code", $code);
		$xml->text($message);
		$xml->endElement();
		$xml->endDocument();

		return $xml->outputMemory();
	}
	
	static public  function diffieHellmanPublicKey($prime, $base, $publicKey) {
		$xml = new XMLWriter;
		$xml->openMemory();
		$xml->startDocument();
		$xml->startElement("neqotiated_dh_key");
		$xml->writeAttribute("dh_prime", $prime);
		$xml->writeAttribute("dh_base", $base);
		$xml->writeAttribute("dh_public_key", $publicKey);
		$xml->endElement();
		$xml->endDocument();

		return $xml->outputMemory();
	}
}

class XMLHandler {
	private $xml;
	private $response;

	public function __construct($input) {
		$this->xml = new XMLReader;
		$this->xml->xml(str_replace("\\\"", "'", $input));
	}

	public function handle() {
		while ($this->xml->read()) { 
			switch ($this->xml->nodeType) { 
				case XMLReader::END_ELEMENT: 
				break;
				case XMLReader::ELEMENT:
					if (strcasecmp($this->xml->name, "iq") == 0) {
						$type = $this->xml->getAttribute("type");
						if ($type != NULL) {
							if (strcasecmp($type, "get") == 0)
								$this->handleIqGet();
						}
					}
				break;
			}
		}
		return $this->response;
	}

	public function handleIqGet() {
		while ($this->xml->read()) { 
			switch ($this->xml->nodeType) { 
				case XMLReader::END_ELEMENT: 
					return;
				case XMLReader::ELEMENT:
					if (strcasecmp($this->xml->name, "query") == 0)
						$this->handleIqGetQuery();
				break;
			}
		} 
	}

	public function handleIqGetQuery() {
		while ($this->xml->read()) { 
			switch ($this->xml->nodeType) { 
				case XMLReader::END_ELEMENT: 
					return;
				case XMLReader::ELEMENT:
					if (strcasecmp($this->xml->name, "commits") == 0) {
						$branch = $this->xml->getAttribute("branch");
						$first = $this->xml->getAttribute("first");
						if ($first != "")
							$first = sha1_bin($first);
						$last = $this->xml->getAttribute("last");
						if ($last != "")
							$last = sha1_bin($last);
						if ($branch !== NULL && $first !== NULL && $last !== NULL) {
							
							$data = $this->syncManager->packMissingDiffs($branch, $first, $last, -1);
							$this->response = XMLResponse::qsGetQueryCommit($data, $branch, sha1_hex($first), sha1_hex($last));
						}
					}
				break;
			}
		} 
	}
}


?>
