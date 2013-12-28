#include "message.h"

Message::Message()
{
}

const char *kFromStanza = "from";
const char *kEncryptionStanza = "crypto";
const char *kMessageSignaturAttr = "signatur";
const char *kAsymmetricKeyIdAttr = "asym";
const char *kEncryptedKeyAttr = "mkey";

/*
Header:
<from id="senderId" key="publicKeyId" signature="messageSignatur" />
<crypto key="receiverPublicKeyId" mkey="encryptedMessageKey"/>
<meta>
XML data encrypted using the mkey.
<parent id="parentMessageId">
<to .../>
</meta>

Body:
Data encrypted using mkey
*/
/*
void Message::makeHeader()
{
    QByteArray outData;
    ProtocolOutStream outStream(&outData);

    OutStanza *fromStanza = new OutStanza(kFromStanza);
    fromStanza->addAttribute("id", );
    fromStanza->addAttribute(kMessageSignaturAttr, );
    outStream.pushChildStanza(fromStanza);

    OutStanza *keyStanza = new OutStanza(kEncryptionStanza);
    keyStanza->addAttribute(kAsymmetricKeyIdAttr, );
    keyStanza->addAttribute(kEncryptedKeyAttr, );
    outStream.pushChildStanza(keyStanza);

    outStream.flush();
}*/


