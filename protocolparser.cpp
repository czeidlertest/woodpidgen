#include "protocolparser.h"


OutStanza::OutStanza(const QString &name) :
    fName(name),
    fParent(NULL)
{
}

const QString &OutStanza::name() const
{
    return fName;
}

const QXmlStreamAttributes &OutStanza::attributes() const
{
    return fAttributes;
}

const QString &OutStanza::text() const
{
    return fText;
}

OutStanza *OutStanza::parent() const
{
    return fParent;
}

void OutStanza::setText(const QString &text)
{
    fText = text;
}

void OutStanza::addAttribute(const QString &namespaceUri, const QString &name, const QString &value)
{
    fAttributes.append(namespaceUri, name, value);
}

void OutStanza::addAttribute(const QString &qualifiedName, const QString &value)
{
    fAttributes.append(qualifiedName, value);
}

void OutStanza::clearData()
{
    fText = "";
    fAttributes = QXmlStreamAttributes();
}

void OutStanza::setParent(OutStanza *parent)
{
    fParent = parent;
}


ProtocolOutStream::ProtocolOutStream(QIODevice *device) :
    fXMLWriter(device),
    fCurrentStanza(NULL)
{
    fXMLWriter.writeStartDocument();
}

ProtocolOutStream::ProtocolOutStream(QByteArray *data) :
    fXMLWriter(data),
    fCurrentStanza(NULL)
{
    fXMLWriter.writeStartDocument();
}

ProtocolOutStream::~ProtocolOutStream()
{
    while (fCurrentStanza != NULL) {
        OutStanza *parent = fCurrentStanza->parent();
        delete fCurrentStanza;
        fCurrentStanza = parent;
    }
}

void ProtocolOutStream::pushStanza(OutStanza *stanza)
{
    if (fCurrentStanza != NULL)
        fXMLWriter.writeEndElement();
    writeStanze(stanza);

    OutStanza *parent = NULL;
    if (fCurrentStanza != NULL) {
        parent = fCurrentStanza->parent();
        delete fCurrentStanza;
    }
    stanza->setParent(parent);
    fCurrentStanza = stanza;
}

void ProtocolOutStream::pushChildStanza(OutStanza *stanza)
{
    writeStanze(stanza);

    stanza->setParent(fCurrentStanza);
    fCurrentStanza = stanza;
}

void ProtocolOutStream::cdDotDot()
{
    if (fCurrentStanza != NULL) {
        fXMLWriter.writeEndElement();
        OutStanza *parent = fCurrentStanza->parent();
        delete fCurrentStanza;
        fCurrentStanza = parent;
    }
}

void ProtocolOutStream::flush()
{
    while (fCurrentStanza != NULL) {
        fXMLWriter.writeEndElement();
        OutStanza *parent = fCurrentStanza->parent();
        delete fCurrentStanza;
        fCurrentStanza = parent;
    }
    fXMLWriter.writeEndDocument();
}

void ProtocolOutStream::writeStanze(OutStanza *stanza)
{
    fXMLWriter.writeStartElement(stanza->name());
    const QXmlStreamAttributes& attributes = stanza->attributes();
    for (int i = 0; i < attributes.count(); i++)
        fXMLWriter.writeAttribute(attributes.at(i));
    if (stanza->text() != "")
        fXMLWriter.writeCharacters(stanza->text());
}


ProtocolInStream::ProtocolInStream(QIODevice *device) :
    fXMLReader(device),
    fRoot(NULL)
{
    fCurrentHandlerTree = &fRoot;
}

ProtocolInStream::ProtocolInStream(const QByteArray &data) :
    fXMLReader(data),
    fRoot(NULL)
{
    fCurrentHandlerTree = &fRoot;
}

void ProtocolInStream::parse()
{
    while (!fXMLReader.atEnd()) {
        switch (fXMLReader.readNext()) {
        case QXmlStreamReader::EndElement: {
            handler_tree *parent = fCurrentHandlerTree->parent;
//            delete fCurrentHandlerTree;
            fCurrentHandlerTree = parent;

            // update handler list
            handler_tree *parentHandler = fCurrentHandlerTree->parent;
            if (parentHandler != NULL) {
                fCurrentHandlerTree->handlers.clear();
                for (int i = 0; i < parentHandler->handlers.count(); i++)
                    fCurrentHandlerTree->handlers.append(parentHandler->handlers.at(i));
            }
            break;
        }

        case QXmlStreamReader::StartElement: {
            QString name = fXMLReader.name().toString();
            handler_tree *handlerTree = new handler_tree(fCurrentHandlerTree);

            for (int i = 0; i < fCurrentHandlerTree->handlers.count(); i++) {
                InStanzaHandler *handler = fCurrentHandlerTree->handlers.at(i);
                if (handler->stanzaName() == name) {
                    handler->handleStanza(fXMLReader.attributes());
                    for (int a = 0; a < handler->childs().count(); a++)
                        handlerTree->handlers.append(handler->childs().at(a));
                    if (handler->isTextRequired())
                        handlerTree->handlers.append(handler);
                }
            }
            fCurrentHandlerTree = handlerTree;
            break;
        }

        case QXmlStreamReader::Characters: {
            for (int i = 0; i < fCurrentHandlerTree->handlers.count(); i++) {
                InStanzaHandler *handler = fCurrentHandlerTree->handlers.at(i);
                if (handler->isTextRequired())
                    handler->handleText(fXMLReader.text());

            }
            break;
        }

        default:
            break;
        }
    }
}

void ProtocolInStream::addHandler(InStanzaHandler *handler)
{
    fRoot.handlers.append(handler);
}


InStanzaHandler::InStanzaHandler(const QString &stanza, bool textRequired) :
    fName(stanza),
    fTextRequired(textRequired),
    fParent(NULL)
{
}

InStanzaHandler::~InStanzaHandler()
{
    for (int i = 0; i < fChildHandlers.count(); i++)
        delete fChildHandlers.at(i);
}

QString InStanzaHandler::stanzaName() const
{
    return fName;
}

bool InStanzaHandler::isTextRequired() const
{
    return fTextRequired;
}

void InStanzaHandler::handleStanza(const QXmlStreamAttributes &/*attributes*/)
{
}

void InStanzaHandler::handleText(const QStringRef &/*text*/)
{
}

void InStanzaHandler::addChildHandler(InStanzaHandler *handler)
{
    fChildHandlers.append(handler);
    handler->setParent(this);
}

InStanzaHandler *InStanzaHandler::parent() const
{
    return fParent;
}

const QList<InStanzaHandler *> &InStanzaHandler::childs() const
{
    return fChildHandlers;
}

void InStanzaHandler::setParent(InStanzaHandler *parent)
{
    fParent = parent;
}

ProtocolInStream::handler_tree::handler_tree(ProtocolInStream::handler_tree *p) :
    parent(p)
{
}

ProtocolInStream::handler_tree::~handler_tree()
{
}


IqOutStanza::IqOutStanza(IqType type) :
    OutStanza("iq"),
    fType(type)
{
    addAttribute("type", toString(fType));
}


IqType IqOutStanza::type()
{
    return fType;
}

QString IqOutStanza::toString(IqType type)
{
    switch (type) {
    case kGet:
        return "get";
    case kSet:
        return "set";
    case kResult:
        return "result";
    case kError:
        return "error";
    default:
        return "bad_type";
    }
    return "";
}


IqInStanzaHandler::IqInStanzaHandler() :
    InStanzaHandler("iq"),
    fType(kBadType)
{
}

IqType IqInStanzaHandler::type()
{
    return fType;
}

void IqInStanzaHandler::handleStanza(const QXmlStreamAttributes &attributes)
{
    if (attributes.hasAttribute("type"))
        fType = fromString(attributes.value("type").toString());
}

IqType IqInStanzaHandler::fromString(const QString &string)
{
    if (string.compare("get", Qt::CaseInsensitive) == 0)
        return kGet;
    if (string.compare("set", Qt::CaseInsensitive) == 0)
        return kSet;
    if (string.compare("result", Qt::CaseInsensitive) == 0)
        return kResult;
    if (string.compare("error", Qt::CaseInsensitive) == 0)
        return kError;
    return kBadType;
}
