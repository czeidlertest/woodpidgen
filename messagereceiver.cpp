#include "messagereceiver.h"

#include <stdio.h>

#include <QXmlStreamAttributes>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>




class XMLHandler {
public:
    XMLHandler(const QByteArray& input, MessageReceiver* receiver)
        :
        fXML(input),
        fMessageReceiver(receiver)
    {
    }

    void handle() {
        while (!fXML.atEnd()) {
            switch (fXML.readNext()) {
            case QXmlStreamReader::EndElement:
                break;
            case QXmlStreamReader::StartElement:
                if (fXML.name().compare("iq", Qt::CaseInsensitive) == 0) {
                    QXmlStreamAttributes attributes = fXML.attributes();
                    if (attributes.hasAttribute("type")) {
                        if (attributes.value("type").compare("result", Qt::CaseInsensitive) == 0)
                            handleIqResult();
                    }

                }
                break;
            default:
                break;
            }
        }

    }

    void handleIqResult() {
        while (!fXML.atEnd()) {
            switch (fXML.readNext()) {
            case QXmlStreamReader::EndElement:
                return;
                break;
            case QXmlStreamReader::StartElement:
                if (fXML.name().compare("query", Qt::CaseInsensitive) == 0) {
                    handleIqResultQuery();
                }
                break;
            default:
                break;
            }
        }

    }

    void handleIqResultQuery() {
        QByteArray packedData;
        while (!fXML.atEnd()) {
            switch (fXML.readNext()) {
            case QXmlStreamReader::EndElement:
                return;
                break;
            case QXmlStreamReader::StartElement:
                if (fXML.name().compare("pack", Qt::CaseInsensitive) == 0) {
                    //TODO handle different package formats...
                }
                break;
            case QXmlStreamReader::Characters:
                packedData = fXML.text().toLocal8Bit();
                fMessageReceiver->commitPackReceived(packedData);
                break;
            default:
                break;
            }
        }

    }


private:
        QXmlStreamReader       fXML;
        MessageReceiver*       fMessageReceiver;
};

MessageReceiver::MessageReceiver(GitInterface* gitInterface, QObject *parent) :
    QObject(parent),
    fGitInterface(gitInterface)
{
}

void MessageReceiver::messageDataReceived(const QByteArray& data)
{
    XMLHandler xmlHandler(data, this);
    xmlHandler.handle();

/*      QString type;
        QString size;
        int objectEnd = objectStart;
        objectEnd = _ReadTill(text, type, objectEnd, ' ');
        printf("type %s\n", type.toStdString().c_str());
        objectEnd = _ReadTill(text, size, objectEnd, '\0');
        //int dataStart = pos;
        printf("size %i\n", size.toInt());
        objectEnd += size.toInt();

        const char* dataPointer = text.data() + objectStart;
        fGitInterface->WriteObject(dataPointer, objectEnd - objectStart);

        objectStart = objectEnd;
*/
/*printf("%s \n", dataPointer - 2);
        QByteArray cdata;
        //cdata = QByteArray::fromBase64(QByteArray::fromRawData(dataPointer, start - blobStart));
        cdata = QByteArray::fromRawData(dataPointer, start - blobStart);
        printf("compresed %i: %s\n", cdata.length(), cdata.data());
        QByteArray uncomp = qUncompress(cdata);
        printf("uncompresed: %s\n", uncomp.data());   */
}

QString MessageReceiver::getMessagesRequest()
{
    return _CreateXMLMessageRequest("", "");
}

void MessageReceiver::commitPackReceived(const QByteArray& data)
{
    QByteArray text = QByteArray::fromBase64(data);

    int objectStart = 0;
    while (objectStart < text.length()) {
        QString hash;
        QString size;
        int objectEnd = objectStart;
        objectEnd = _ReadTill(text, hash, objectEnd, ' ');
        printf("hash %s\n", hash.toStdString().c_str());
        objectEnd = _ReadTill(text, size, objectEnd, '\0');
        int blobStart = objectEnd;
        printf("size %i\n", size.toInt());
        objectEnd += size.toInt();

        const char* dataPointer = text.data() + blobStart;
        fGitInterface->WriteFile(hash, dataPointer, objectEnd - blobStart);

        objectStart = objectEnd;
    }
}


int MessageReceiver::_ReadTill(QByteArray& in, QString &out, int start, char stopChar)
{
    int pos = start;
    while (pos < in.length() && in.at(pos) != stopChar) {
        out.append(in.at(pos));
        pos++;
    }
    pos++;
    return pos;
}

QString MessageReceiver::_CreateXMLMessageRequest(const QString &fromCommit, const QString &toCommit)
{
    QString output;
    QXmlStreamWriter xmlWriter(&output);
    xmlWriter.writeStartDocument();

    xmlWriter.writeStartElement("iq");
    xmlWriter.writeAttribute("type", "get");
    xmlWriter.writeStartElement("query");
    xmlWriter.writeAttribute("xmlns", "git:transfer");
    xmlWriter.writeStartElement("commits");
    xmlWriter.writeAttribute("branch", "master");
    xmlWriter.writeAttribute("first", fromCommit);
    xmlWriter.writeAttribute("last", toCommit);
    xmlWriter.writeEndElement();
    xmlWriter.writeEndElement();
    xmlWriter.writeEndElement();

    xmlWriter.writeEndDocument();
    printf("xml: %s\n", output.toStdString().c_str());
    return output;
}
