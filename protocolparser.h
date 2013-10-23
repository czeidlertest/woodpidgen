#ifndef PROTOCOLPARSER_H
#define PROTOCOLPARSER_H

#include <QXmlStreamAttributes>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>


class Stanza {
public:
    Stanza(const QString &elementName, Stanza *parent = NULL);
    virtual ~Stanza();

    const QString &name() const;
    Stanza *parent() const;

    Stanza *addChild(const QString &elementName);
    bool addChild(Stanza *child);
    const QList<Stanza*> &childs() const;

    void setText(const QString &text);
    const QString& text();

    const QXmlStreamAttributes& attributes() const;
    void addAttribute(const QString &namespaceUri, const QString &name, const QString &value);
    void addAttribute(const QString &qualifiedName, const QString &value);
protected:
    void setParent(Stanza *parent);

    QString fElementName;
    Stanza *fParent;
    QXmlStreamAttributes fAttributes;
    QList<Stanza*> fChilds;
    QString fText;
};


class StanzaView {
public:
    StanzaView(const Stanza *stanza);

    const QString &name() const;
    Stanza *parent() const;
    const QList<Stanza*> &childs() const;
    const QXmlStreamAttributes& attributes() const;

private:
    const Stanza *fStanza;
};

class IqStanza : public Stanza {
public:
    enum IqType {
        kGet,
        kSet,
        kResult,
        kError,
        kBadType
    };

    IqStanza(IqType type);

    //! tries to read from a generic DataEntry
    static IqStanza* fromStanza(Stanza* entry);

    IqType type();

private:
    static QString toString(IqType type);
    static IqType fromString(const QString &string);

    IqType fType;
};

class ProtocolParser
{
public:
    static Stanza* parse(const QByteArray& input);
    static bool write(QString &output, Stanza* rootEntry);
private:
    static void writeDataEntry(QXmlStreamWriter &xmlWriter, Stanza *entry);
};


class ProtocolInStreamHandler {
    typedef bool (ProtocolInStreamHandler::*handleStanza)() const;
};


class ProtocolInStream {
public:
    void parse();

    //void registerHandler(const QString &stanza, )

};

#endif // PROTOCOLPARSER_H
