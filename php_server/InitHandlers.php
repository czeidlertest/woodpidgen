<?php

include_once './SyncHandler.php';
include_once './AuthHandler.php';


function initSyncHandlers($XMLHandler) {
	$userDir = Session::get()->getUserDir();
	if (!file_exists ($userDir))
		mkdir($userDir);
	$database = new GitDatabase($userDir."/.git");

	// pull
	$pullIqGetHandler = new InIqStanzaHandler(IqType::$kGet);
	$pullHandler = new SyncPullStanzaHandler($XMLHandler->getInStream(), $database);
	$pullIqGetHandler->addChild($pullHandler);
	$XMLHandler->addHandler($pullIqGetHandler);

	// push
	$pushIqGetHandler = new InIqStanzaHandler(IqType::$kSet);
	$pushHandler = new SyncPushStanzaHandler($XMLHandler->getInStream(), $database);
	$pushIqGetHandler->addChild($pushHandler);
	$XMLHandler->addHandler($pushIqGetHandler);
}

function initAuthHandlers($XMLHandler) {
	// auth
	$userAuthIqGetHandler = new InIqStanzaHandler(IqType::$kSet);
	$userAuthHandler = new UserAuthStanzaHandler($XMLHandler->getInStream());
	$userAuthIqGetHandler->addChild($userAuthHandler);
	$XMLHandler->addHandler($userAuthIqGetHandler);

	$userAuthSignatureIqGetHandler = new InIqStanzaHandler(IqType::$kSet);
	$userAuthSignatureHandler = new UserAuthSignedStanzaHandler($XMLHandler->getInStream());
	$userAuthSignatureIqGetHandler->addChild($userAuthSignatureHandler);
	$XMLHandler->addHandler($userAuthSignatureIqGetHandler);
		
	$logoutIqGetHandler = new InIqStanzaHandler(IqType::$kSet);
	$logoutHandler = new LogoutStanzaHandler($XMLHandler->getInStream());
	$logoutIqGetHandler->addChild($logoutHandler);
	$XMLHandler->addHandler($logoutIqGetHandler);
}

class InitHandlers {
	static public function initPrivateHandlers($XMLHandler) {
		initSyncHandlers($XMLHandler);
		initAuthHandlers($XMLHandler);
	}
	
	static public function initPublicHandlers($XMLHandler) {
		initAuthHandlers($XMLHandler);
	}
}

?>
