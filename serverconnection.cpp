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
    fFilter(NULL)
{
}

RemoteConnection::~RemoteConnection()
{
    delete fFilter;
}

void RemoteConnection::setFilter(RemoteConnectionFilter *filter)
{
    fFilter = filter;
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

int NetworkConnection::send(RemoteConnectionReply *remoteConnectionReply, const QByteArray &data)
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
        return -1;

    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));

    fReplyMap.insert(reply, remoteConnectionReply);
    return 0;
}

void NetworkConnection::replyFinished(QNetworkReply *reply)
{
    QMap<QNetworkReply*, RemoteConnectionReply*>::iterator it = fReplyMap.find(reply);
    RemoteConnectionReply *receiver = it.value();

    if (fFilter != NULL) {
        QByteArray incoming;
        fFilter->receiveFilter(reply->readAll(), incoming);
        receiver->received(incoming);
    } else {
        receiver->received(reply->readAll());
    }

    fReplyMap.remove(reply);
}


EncryptedPHPConnection::EncryptedPHPConnection(QUrl url, CryptoInterface *crypto, QObject *parent) :
    NetworkConnection(url, parent),
    fCrypto(crypto)
{
}


int EncryptedPHPConnection::connectToServer()
{
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

    //PHPEncryptionFilter* filter = new PHPEncryptionFilter();
    //fConnection->setFilter(filter);
    return 0;
}


#include <QDebug>
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
    qDebug() << "Data :" << data << endl;

    if (prime == "" || base == "" || publicKey == "")
        return;
    SecureArray key = fCrypto->sharedDHKey(prime, publicKey, fSecretNumber);

    qDebug() << "shared key: " <<  key.data() << endl;

    PHPEncryptionFilter *filter = new PHPEncryptionFilter(fCrypto, key, fInitVector);
    setFilter(filter);

    fNetworkReply->deleteLater();
    emit connectionAttemptFinished(QNetworkReply::NoError);
}

void EncryptedPHPConnection::networkRequestError(QNetworkReply::NetworkError code)
{
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
    fCrypto->encryptSymmetric(in, out, fCipherKey, fIV);
}


void EncryptedPHPConnection::PHPEncryptionFilter::receiveFilter(const QByteArray &in, QByteArray &out)
{
    fCrypto->decryptSymmetric(in, out, fCipherKey, fIV);
}
