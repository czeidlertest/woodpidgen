<?php
#if(rand(1,3) == 1){
    /* Fake an error */
 #   header("HTTP/1.0 404 Not Found");
  #  die();
#}

/* Send a string after a random number of seconds (2-10) */
#sleep(rand(0,1));

include_once './database.php';


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
        $this->attributes[] = new XMLAttribute($namespaceUri, $name, $value);
    }

    public function addAttribute($name, $value) {
        $this->attributes[] = new XMLAttribute("", $name, $value);
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
        $this->writeStanza($stanza);

        if ($this->currentStanza != NULL)
            $stanza->setParent($this->currentStanza->parent());

        $this->currentStanza = $stanza;
    }

    public function pushChildStanza($stanza) {
        $this->writeStanza($stanza);

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

    private function writeStanza($stanza) {
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


class IqErrorOutStanza extends IqOutStanza {
	public function __construct($details = "") {
		IqOutStanza::__construct(IqType::$kError);
		if ($details != "")
			$this->addAttribute("details", $details);
	}
	
	public static function makeErrorMessage($details) {
		$outStream = new ProtocolOutStream();
		$outStream->pushStanza(new IqErrorOutStanza($details));
		return $outStream->flush();
	}
}


class InStanzaHandler {
	private $name;
	private $optional;
	private $hasBeenHandled;
	private $childHandlers;

	public function __construct($name) {
		$this->name = $name;
		$this->optional = false;
		$this->hasBeenHandled = false;
		$this->childHandlers = array();
	}

	public function getName() {
		return $this->name;
	}

	public function isOptional() {
		return $this->optional;
	}

	public function setOptional($optional) {
		$this->optional = $optional;
	}

	public function handleStanza($attributes) {
		return false;
	}
	
	public function handleText($text) {
		return false;
	}

	public function finished() {
	}

	public function setHandled($handled = true) {
		$this->hasBeenHandled = $handled;
	}

	public function hasBeenHandled() {
		if (!$this->hasBeenHandled)
			return false;
		
		foreach ($this->childHandlers as $child) {
			if (!$child->isOptional() && !$child->hasBeenHandled())
				return false;
		}
		return true;
	}

	public function addChild($child, $optional = false) {
		$child->optional = $optional;
		$this->childHandlers[] = $child;
	}

	public function getChildHandlers() {
		return $this->childHandlers;
	}
};


class handler_tree {
	public $parentTree;
	public $handlers;
	
	public function __construct($parent) {
		$this->parentTree = $parent;
		$this->handlers = array();
	}
};
    
    
class ProtocolInStream {
	private $currentHandler = array();
	private $xml;
	private $currentHandlerTree;
	private $root;
	private $response;
	
	public function __construct($input) {
		$this->xml = new XMLReader;
		$this->xml->xml(str_replace("\\\"", "'", $input));
		$this->root = new handler_tree(null);
		$this->currentHandlerTree = $this->root;
	}

	public function appendResponse($response) {
		$this->response = $this->response.$response;
	}

	public function parse() {
		while ($this->xml->read()) { 
			switch ($this->xml->nodeType) {
				case XMLReader::END_ELEMENT:
					foreach ($this->currentHandlerTree->handlers as $handler) {
						if ($handler->hasBeenHandled())
							$handler->finished();
					}

					$parent = $this->currentHandlerTree->parentTree;
					$this->currentHandlerTree = $parent;

					// update handler list
					$parentHandler = $this->currentHandlerTree->parentTree;
					if ($parentHandler != null) {
						$this->currentHandlerTree->handlers = array();
						foreach ($parentHandler->handlers as $handler) {
							$this->currentHandlerTree->handlers = array_merge(
								$this->currentHandlerTree->handlers, $handler->getChildHandlers());
						}
					}
					break;

				case XMLReader::ELEMENT:
					$handlerTree = new handler_tree($this->currentHandlerTree);
					foreach ($this->currentHandlerTree->handlers as $handler) {
						if ($handler->getName() == $this->xml->name) {
							$handeled = $handler->handleStanza($this->xml);
							$handler->setHandled($handeled);
							if (!$handeled && !$handler->isOptional())
								continue;
							foreach ($handler->getChildHandlers() as $child)
								$handlerTree->handlers[] = $child;
						}
					}
					$this->currentHandlerTree = $handlerTree;
					break;
			}
		}
		return $this->response;
	}

	public function addHandler($handler, $optional = true) {
		$handler->setOptional($optional);
		$this->root->handlers[] = $handler;
	}
}


class InIqStanzaHandler extends InStanzaHandler {
	private $type;
	public function __construct($type) {
		InStanzaHandler::__construct("iq");
		$this->type = $type;
	}

	public function getType() {
		return $this->type;
	}

	public function handleStanza($xml) {
		$typeString = $xml->getAttribute("type");
		if ($typeString === null)
			return false;
		$type = $this->fromString($typeString);
		if ($this->type != $type)
			return false;
		return true;
	}

	private function fromString($type) {
		if (strcasecmp($type, "get") == 0)
			return IqType::$kGet;
		if (strcasecmp($type, "set") == 0)
			return IqType::$kSet;
		if (strcasecmp($type, "result") == 0)
			return IqType::$kResult;
		if (strcasecmp($type, "error") == 0)
			return IqType::$kError;
		return IqType::$kBadType;
    }
}

function url_decode($data) {
	$data = str_replace(" ", "+", $data);
	return base64_decode($data);
}

// sync handler


class SyncPullStanzaHandler extends InStanzaHandler {
	private $inStreamReader;
	private $database;
	public function __construct($inStreamReader, $database) {
		InStanzaHandler::__construct("sync_pull");
		$this->inStreamReader = $inStreamReader;
		$this->database = $database;
	}

	public function getType() {
		return $this->type;
	}

	public function handleStanza($xml) {
		$branch = $xml->getAttribute("branch");
		$remoteTip = $xml->getAttribute("tip");
		if ($branch === null || $remoteTip === null)
			return;
		if (isSHA1Hex($remoteTip))
			$remoteTip = sha1_bin($remoteTip);

		$packManager = new PackManager($this->database);
		$pack = "";
		try {
			$localTip = $this->database->getTip($branch);
			$pack = $packManager->exportPack($branch, $remoteTip, $localTip, -1);
		} catch (Exception $e) {
			$localTip = "";
		}

		// produce output
		$outStream = new ProtocolOutStream();
		$outStream->pushStanza(new IqOutStanza(IqType::$kResult));

		$stanza = new OutStanza("sync_pull");
		$stanza->addAttribute("branch", $branch);
		$localTipHex = "";
		if (strlen($localTip) == 20)
			$localTipHex = sha1_hex($localTip);
		$stanza->addAttribute("tip", $localTipHex);
		$outStream->pushChildStanza($stanza);
		
		$packStanza = new OutStanza("pack");
		$packStanza->setText(base64_encode($pack));
		$outStream->pushChildStanza($packStanza);
		
		$this->inStreamReader->appendResponse($outStream->flush());
		return true;
	}
}

class SyncPushPackHandler extends InStanzaHandler {
	private $pack;

	public function __construct() {
		InStanzaHandler::__construct("pack");
	}
	
	public function handleStanza($xml) {
		$this->pack = url_decode($xml->readString());
		return true;
	}
	
	public function getPack() {
		return $this->pack;
	}
}

class SyncPushStanzaHandler extends InStanzaHandler {
	private $inStreamReader;
	private $database;
	private $packHandler;
	
	private $branch;
	private $startCommit;
	private $lastCommit;

	public function __construct($inStreamReader, $database) {
		InStanzaHandler::__construct("sync_push");
		$this->inStreamReader = $inStreamReader;
		$this->database = $database;
	}

	public function getType() {
		return $this->type;
	}

	public function handleStanza($xml) {
		$this->branch = $xml->getAttribute("branch");
		$this->startCommit = $xml->getAttribute("start_commit");
		$this->lastCommit = $xml->getAttribute("last_commit");
		$this->packHandler = new SyncPushPackHandler();
		$this->addChild($this->packHandler);
		return true;
	}
	
	public function finished() {
		$pack = $this->packHandler->getPack();
		
		$packManager = new PackManager($this->database);
		if (!$packManager->importPack($this->branch, $pack, $this->startCommit, $this->lastCommit)) {
			$this->inStreamReader->appendResponse(IqErrorOutStanza::makeErrorMessage("Push: unable to import pack."));
			return;
		}
		
		// produce output
		$outStream = new ProtocolOutStream();
		$outStream->pushStanza(new IqOutStanza(IqType::$kResult));

		$stanza = new OutStanza("sync_push");
		$stanza->addAttribute("branch", $this->branch);
		$localTip = sha1_hex($this->database->getTip($this->branch));
		$stanza->addAttribute("tip", $localTip);
		$outStream->pushChildStanza($stanza);

		$this->inStreamReader->appendResponse($outStream->flush());
	}
}

class XMLHandler {
	private $inStream;
	private $response;
	private $database;

	public function __construct($input) {
		$this->inStream = new ProtocolInStream($input);
		$this->database = new GitDatabase(".git");
		$this->initPrivateHandlers();
	}
	
	public function initPrivateHandlers() {
		// pull
		$pullIqGetHandler = new InIqStanzaHandler(IqType::$kGet);
		$pullHandler = new SyncPullStanzaHandler($this->inStream, $this->database);
		$pullIqGetHandler->addChild($pullHandler);
		$this->inStream->addHandler($pullIqGetHandler);
		
		// push
		$pushIqGetHandler = new InIqStanzaHandler(IqType::$kSet);
		$pushHandler = new SyncPushStanzaHandler($this->inStream, $this->database);
		$pushIqGetHandler->addChild($pushHandler);
		$this->inStream->addHandler($pushIqGetHandler);
	}

	public function handle() {
		return $this->inStream->parse();
	}
}

?>
