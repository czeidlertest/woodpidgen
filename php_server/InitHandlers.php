<?php

include_once './SyncHandler.php';
include_once './AuthHandler.php';


class InitHandlers {
	static public function initPrivateHandlers($XMLHandler) {
		// pull
		$pullIqGetHandler = new InIqStanzaHandler(IqType::$kGet);
		$pullHandler = new SyncPullStanzaHandler($XMLHandler->getInStream(), $XMLHandler->getDatabase());
		$pullIqGetHandler->addChild($pullHandler);
		$XMLHandler->addHandler($pullIqGetHandler);

		// push
		$pushIqGetHandler = new InIqStanzaHandler(IqType::$kSet);
		$pushHandler = new SyncPushStanzaHandler($XMLHandler->getInStream(), $XMLHandler->getDatabase());
		$pushIqGetHandler->addChild($pushHandler);
		$XMLHandler->addHandler($pushIqGetHandler);

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
}

?>
