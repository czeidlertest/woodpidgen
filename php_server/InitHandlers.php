<?php

include_once './AuthHandler.php';
include_once './MessageHandler.php';
include_once './SyncHandler.php';


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

function initAccountAuthHandler($XMLHandler) {
	$iqHandler = new InIqStanzaHandler(IqType::$kSet);
	$handler = new AccountAuthStanzaHandler($XMLHandler->getInStream());
	$iqHandler->addChild($handler);
	$XMLHandler->addHandler($iqHandler);
}

function initAccountAuthSignedHandler($XMLHandler) {
	$iqHandler = new InIqStanzaHandler(IqType::$kSet);
	$handler = new AccountAuthSignedStanzaHandler($XMLHandler->getInStream());
	$iqHandler->addChild($handler);
	$XMLHandler->addHandler($iqHandler);
}

function initAuthHandlers($XMLHandler) {
	// auth
	initAccountAuthHandler($XMLHandler);
	initAccountAuthSignedHandler($XMLHandler);	

	$logoutIqSetHandler = new InIqStanzaHandler(IqType::$kSet);
	$logoutHandler = new LogoutStanzaHandler($XMLHandler->getInStream());
	$logoutIqSetHandler->addChild($logoutHandler);
	$XMLHandler->addHandler($logoutIqSetHandler);
}

function initMessageHandlers($XMLHandler) {
	$iqHandler = new InIqStanzaHandler(IqType::$kSet);
	$handler = new MessageStanzaHandler($XMLHandler->getInStream());
	$iqHandler->addChild($handler);
	$XMLHandler->addHandler($iqHandler);	
}


class InitHandlers {
	static public function initPrivateHandlers($XMLHandler) {
		initSyncHandlers($XMLHandler);
		initAuthHandlers($XMLHandler);
		initMessageHandlers($XMLHandler);
	}
	
	static public function initPublicHandlers($XMLHandler) {
		initAuthHandlers($XMLHandler);
		initMessageHandlers($XMLHandler);
	}
}

?>
