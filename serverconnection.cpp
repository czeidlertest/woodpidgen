#include "serverconnection.h"

#include <QCoreApplication>
#include <QXmlStreamAttributes>
#include <QXmlStreamReader>

#include <mainapplication.h>



RemoteConnectionReply::RemoteConnectionReply(QObject *parent) :
    QObject(parent)
{
}


RemoteConnection::RemoteConnection(QObject *parent) :
    QObject(parent),
    fConnected(false),
    fConnecting(false),
    fFilter(NULL)
{
}

RemoteConnection::~RemoteConnection()
{
    delete fFilter;
}

bool RemoteConnection::isConnected()
{
    return fConnected;
}

bool RemoteConnection::isConnecting()
{
    return fConnecting;
}

void RemoteConnection::setFilter(RemoteConnectionFilter *filter)
{
    fFilter = filter;
}

void RemoteConnection::setConnectionStarted()
{
    fConnecting = true;
    fConnected = false;
}

void RemoteConnection::setConnected()
{
    fConnected = true;
    fConnecting = false;
}

void RemoteConnection::setDisconnected()
{
    fConnecting = false;
    fConnected = false;
}


NetworkConnection::NetworkConnection(const QUrl &url, QObject *parent) :
    RemoteConnection(parent),
    fUrl(url)
{
}

NetworkConnection::~NetworkConnection()
{
}

QUrl NetworkConnection::getUrl()
{
    return fUrl;
}

QNetworkAccessManager* NetworkConnection::getNetworkAccessManager()
{
    return ((MainApplication*)qApp)->getNetworkAccessManager();
}

WP::err NetworkConnection::send(RemoteConnectionReply *remoteConnectionReply, const QByteArray &data)
{
    QNetworkRequest request;
    request.setUrl(fUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");

    QNetworkAccessManager *manager = getNetworkAccessManager();
    QNetworkReply *reply = NULL;
    if (fFilter != NULL) {
        QByteArray outgoing;
        fFilter->sendFilter(data, outgoing);
        outgoing.prepend("request=");
        reply = manager->post(request, outgoing);
    } else {
        reply = manager->post(request, data);
    }
    if (reply == NULL)
        return WP::kError;

    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(replyError(QNetworkReply::NetworkError)));

    fReplyMap.insert(reply, remoteConnectionReply);
    return WP::kOk;
}

void NetworkConnection::replyFinished(QNetworkReply *reply)
{
    QMap<QNetworkReply*, RemoteConnectionReply*>::iterator it = fReplyMap.find(reply);
    RemoteConnectionReply *receiver = it.value();

    QByteArray incoming = reply->readAll();
    if (fFilter != NULL) {
        QByteArray filtered;
        fFilter->receiveFilter(incoming, filtered);
        receiver->received(filtered);
    } else {
        receiver->received(incoming);
    }

    fReplyMap.remove(reply);
}

void NetworkConnection::replyError(QNetworkReply::NetworkError code)
{
    qDebug() << "NetworkConnection::replyError: " << code << endl;
}


EncryptedPHPConnection::EncryptedPHPConnection(QUrl url, QObject *parent) :
    NetworkConnection(url, parent)
{
    fCrypto = CryptoInterfaceSingleton::getCryptoInterface();
}


WP::err EncryptedPHPConnection::connectToServer()
{
    if (isConnected())
        return WP::kIsConnected;
    if (isConnecting())
        return WP::kOk;

    fInitVector = fCrypto->generateInitalizationVector(512);
    QString prime, base, pub;
    fCrypto->generateDHParam(prime, base, fSecretNumber, pub);

    QNetworkAccessManager *manager = NetworkConnection::getNetworkAccessManager();

    QNetworkRequest request;
    request.setUrl(fUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");

    QByteArray content = "";
    content += "request=neqotiate_dh_key&";
    content += "dh_prime=" + prime + "&";
    content += "dh_base=" + base + "&";
    content += "dh_public_key=" + pub + "&";
    content += "encrypt_iv=" + fInitVector.toBase64();

    fNetworkReply = manager->post(request, content);
    connect(fNetworkReply, SIGNAL(finished()), this, SLOT(replyFinished()));
    connect(fNetworkReply, SIGNAL(error(QNetworkReply::NetworkError)), this,
            SLOT(networkRequestError(QNetworkReply::NetworkError)));

    setConnectionStarted();
    return WP::kOk;
}

WP::err EncryptedPHPConnection::disconnect()
{
    // TODO notify the server
    setDisconnected();
    return WP::kOk;
}


void EncryptedPHPConnection::replyFinished()
{
    QByteArray data = fNetworkReply->readAll();

    QString prime;
    QString base;
    QString publicKey;

    QXmlStreamReader readerXML(data);
    while (!readerXML.atEnd()) {
        switch (readerXML.readNext()) {
        case QXmlStreamReader::EndElement:
            break;
        case QXmlStreamReader::StartElement:
            if (readerXML.name().compare("neqotiated_dh_key", Qt::CaseInsensitive) == 0) {
                QXmlStreamAttributes attributes = readerXML.attributes();
                if (attributes.hasAttribute("dh_prime"))
                    prime = attributes.value("dh_prime").toString();;
                if (attributes.hasAttribute("dh_base"))
                    base = attributes.value("dh_base").toString();;
                if (attributes.hasAttribute("dh_public_key"))
                    publicKey = attributes.value("dh_public_key").toString();;

            }
            break;
        default:
            break;
        }
    }
    if (prime == "" || base == "" || publicKey == "")
        return;
    SecureArray key = fCrypto->sharedDHKey(prime, publicKey, fSecretNumber);
    // make it at least 128 byte
    for (int i = key.count(); i < 128; i++)
        key.append('\0');

    PHPEncryptionFilter *filter = new PHPEncryptionFilter(fCrypto, key, fInitVector);
    setFilter(filter);

    fNetworkReply->deleteLater();
    setConnected();
    emit connectionAttemptFinished(QNetworkReply::NoError);
}

void EncryptedPHPConnection::networkRequestError(QNetworkReply::NetworkError code)
{
    setDisconnected();
    fNetworkReply->deleteLater();
    emit connectionAttemptFinished(code);
}


EncryptedPHPConnection::PHPEncryptionFilter::PHPEncryptionFilter(CryptoInterface *crypto,
                                                                const SecureArray &cipherKey,
                                                                const QByteArray &iv) :
    fCrypto(crypto),
    fCipherKey(cipherKey),
    fIV(iv)
{
}

void EncryptedPHPConnection::PHPEncryptionFilter::sendFilter(const QByteArray &in, QByteArray &out)
{
    qDebug() << "Key: " << fCipherKey.toBase64().data() << " iv: " << fIV.toBase64().data() << endl;
    fCrypto->encryptSymmetric(in, out, fCipherKey, fIV, "aes128");
    out = out.toBase64();
}


void EncryptedPHPConnection::PHPEncryptionFilter::receiveFilter(const QByteArray &in, QByteArray &out)
{
    QByteArray raw = QByteArray::fromBase64(in);
    fCrypto->decryptSymmetric(raw, out, fCipherKey, fIV, "aes128");
}
