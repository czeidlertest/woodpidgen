#include "protocolparser.h"

Stanza::Stanza(const QString &elementName, Stanza *parent) :
    fElementName(elementName),
    fParent(parent)
{
}

Stanza::~Stanza()
{
    for (int i = 0; i < fChilds.count(); i++)
        delete fChilds.at(i);
}

Stanza *Stanza::parent() const
{
    return fParent;
}

Stanza *Stanza::addChild(const QString &elementName)
{
    if (fText != "")
        return NULL;
    Stanza *entry = new Stanza(elementName, this);
    fChilds.append(entry);
    return entry;
}

bool Stanza::addChild(Stanza *child)
{
    if (fText != "")
        return false;
    child->setParent(this);
    fChilds.append(child);
    return true;
}

const QList<Stanza *> &Stanza::childs() const
{
    return fChilds;
}

void Stanza::setText(const QString &text)
{
    if (fChilds.count() == 0)
        fText = text;
}

const QString &Stanza::text()
{
    return fText;
}

const QString &Stanza::name() const
{
    return fElementName;
}

const QXmlStreamAttributes &Stanza::attributes() const
{
    return fAttributes;
}

void Stanza::addAttribute(const QString &namespaceUri, const QString &name, const QString &value)
{
    fAttributes.append(namespaceUri, name, value);
}

void Stanza::addAttribute(const QString &qualifiedName, const QString &value)
{
    fAttributes.append(qualifiedName, value);
}

void Stanza::setParent(Stanza *parent)
{
    fParent = parent;
}


Stanza *ProtocolParser::parse(const QByteArray &input)
{
    QXmlStreamReader reader(input);
    Stanza *current = NULL;
    while (!reader.atEnd()) {
        switch (reader.readNext()) {
        case QXmlStreamReader::EndElement:
            if (current != NULL)
                current = current->parent();
            break;
        case QXmlStreamReader::StartElement:
            if (current != NULL)
                current = current->addChild(reader.name().toLatin1());
            else
                current = new Stanza(reader.name().toLatin1());
            break;
        case QXmlStreamReader::Characters:
            if (current != NULL)
                current->setText(reader.text().toLatin1());
            break;
        default:
            break;
        }
    }
    return current;
}


void ProtocolParser::writeDataEntry(QXmlStreamWriter &xmlWriter, Stanza *entry)
{
    xmlWriter.writeStartElement(entry->name());
    const QXmlStreamAttributes& attributes = entry->attributes();
    for (int i = 0; i < attributes.count(); i++)
        xmlWriter.writeAttribute(attributes.at(i));
    for (int i = 0; i < entry->childs().count(); i++)
        writeDataEntry(xmlWriter, entry->childs().at(i));
    xmlWriter.writeCharacters(entry->text());
    xmlWriter.writeEndElement();
}

bool ProtocolParser::write(QString &output, Stanza *rootEntry)
{
    if (rootEntry == NULL)
        return false;

    QXmlStreamWriter xmlWriter(&output);
    xmlWriter.writeStartDocument();

    writeDataEntry(xmlWriter, rootEntry);

    xmlWriter.writeEndDocument();
    return true;
}


IqStanza::IqStanza(IqType type) :
    Stanza("iq"),
    fType(type)
{
    fAttributes.append("type", toString(fType));
}

IqStanza *IqStanza::fromStanza(Stanza *entry)
{
    if (entry == NULL)
        return NULL;

    if (entry->name().compare("iq", Qt::CaseInsensitive) != 0)
        return NULL;
    if (!entry->attributes().hasAttribute("type"))
        return NULL;
    IqType type = fromString(entry->attributes().value("type").toString());
    if (type == kBadType)
        return NULL;
    IqStanza *iqStanza = new IqStanza(type);
    //*iqStanza = *entry;
    return iqStanza;
}

IqStanza::IqType IqStanza::type()
{
    return fType;
}

QString IqStanza::toString(IqType type)
{
    switch (type) {
    case kGet:
        return "get";
    case kSet:
        return "set;";
    case kResult:
        return "result";
    case kError:
        return "error";
    default:
        return "bad_type";
    }
    return "";
}

IqStanza::IqType IqStanza::fromString(const QString &string)
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

