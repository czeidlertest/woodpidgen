<?php
#if(rand(1,3) == 1){
    /* Fake an error */
 #   header("HTTP/1.0 404 Not Found");
  #  die();
#}

/* Send a string after a random number of seconds (2-10) */
#sleep(rand(0,1));

include_once './database.php';

#$repoPath = 'test.git';
#$repository = new Git($repoPath);
#$syncManager = new MessageSyncManager($repository);

#$XMLHandler = new XMLHandler($_POST["request"], $syncManager);
#$XMLHandler->handle();


class XMLAttribute {
    public function __construct($namespaceUri, $name, $value) {
        $this->namespace = $namespaceUri;
        $this->name = $name;
        $this->value = $value;
    }

    public $namespace;
    public $name;
    public $value;
}

class OutStanza {
    private $name;
    private $attributes;
    private $text;
    private $parent;

    public function __construct($name) {
        $this->name =$name;
        $this->attributes = array();
        $this->parent = NULL;
    }

    public function name() {
        return $this->name;
    }

    public function attributes() {
        return $this->attributes;
    }

    public function text() {
        return $this->text;
    }

    public function parent() {
        return $this->parent;
    }

    public function setText($text) {
        $this->text = $text;
    }

    public function addAttributeNS($namespaceUri, $name, $value) {
        $this->attibutes[] = new XMLAttribute($namespaceUri, $name, $value);
    }

    public function addAttribute($name, $value) {
        $this->attibutes[] = new XMLAttribute("", $name, $value);
    }

    public function setParent($parent) {
        $this->parent = $parent;
    }
};

class ProtocolOutStream {
    private $xml;
    private $currentStanza;

    public function __construct() {
        $this->xml = new XMLWriter;
        $this->currentStanza = NULL;
        $this->xml->openMemory();
    }

    public function pushStanza($stanza) {
        if ($this->currentStanza != NULL)
            $this->xml->endElement();
        writeStanze($stanza);

        if ($this->currentStanza != NULL)
            $stanza->setParent($this->currentStanza->parent());

        $this->currentStanza = stanza;
    }

    public function pushChildStanza($stanza) {
        writeStanze($stanza);

        $stanza->setParent($this->currentStanza);
        $this->currentStanza = $stanza;
    }

    public function cdDotDot() {
        if ($this->currentStanza != NULL) {
            $this->xml->endElement();
            $this->currentStanza = $this->currentStanza->parent();
        }
    }

    public function flush() {
        while ($this->currentStanza != NULL) {
            $this->xml->endElement();
            $this->currentStanza = $this->currentStanza->parent();
        }
        $this->xml->endDocument();
        return $this->xml->outputMemory();
    }

    private function writeStanze($stanza) {
        $this->xml->startElement($stanza->name());

        foreach ($stanza->attributes() as $attribute) {
            if ($attribute->namespace != "")
                $this->xml->writeAttributeNs("", $attribute->name, $attribute->namespace, $attribute->value);
            else
                $this->xml->writeAttribute($attribute->name, $attribute->value);
        }
        if ($stanza->text() != "")
            $this->xml->text($stanza->text());
    }
};


class IqType {
    public static $kGet = 0;
    public static $kSet = 1;
    public static $kResult = 2;
    public static $kError = 3;
    public static $kBadType = -1;
};


class IqOutStanza extends OutStanza {
    private $type;

    public function __construct($type) {
        OutStanza::__construct("iq");
        $this->type = $type;
        $this->addAttribute("type", $this->toString($type));
    }

    public function type() {
        return $this->type;
    }

    private function toString($type) {
        switch ($type) {
        case IqType::$kGet:
            return "get";
        case IqType::$kSet:
            return "set;";
        case IqType::$kResult:
            return "result";
        case IqType::$kError:
            return "error";
        default:
            return "bad_type";
        }
        return "";
    }
};


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

    private $database;

	public function __construct($input) {
		$this->xml = new XMLReader;
		$this->xml->xml(str_replace("\\\"", "'", $input));

        $this->database = new GitDatabase(".git");
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
					if (strcasecmp($this->xml->name, "sync_pull") == 0)
						$this->handleIqGetSync();
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
	
	public function handleIqGetSync() {
        $branch = $this->xml->getAttribute("branch");
        $tip = $this->xml->getAttribute("tip");
        if ($branch == NULL || $tip == NULL)
            return;

        $packManager = new PackManager($this->database);
        $localTip = $this->database->getTip($branch);
        $pack = $packManager->exportPack($branch, sha1_bin($tip), $localTip, -1);

        // produce output
        $outStream = ProtocolOutStream();
        $outStream->pushStanza(new IqOutStanza(IqType::$kResult));

        $stanza = new OutStanza("sync_pull");
        $stanza->addAttribute("branch", $branch);
        $stanza->addAttribute("tip", $localTip);

        $outStream->pushChildStanza($stanza);
        $this->response = $outStream->flush();
	}
}


?>
