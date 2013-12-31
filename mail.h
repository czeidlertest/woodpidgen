#ifndef MAIL_H
#define MAIL_H

#include <QByteArray>

class RawMailMessage {
public:
    RawMailMessage(const QString &header, const QString &body);
    RawMailMessage();

    const QByteArray& getHeader() const;
    const QByteArray& getBody() const;

    QByteArray& getHeader();
    QByteArray& getBody();

private:
    QByteArray fHeader;
    QByteArray fBody;
};


#endif // MAIL_H
