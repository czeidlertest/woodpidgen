#include "mail.h"

#include <QString>

RawMailMessage::RawMailMessage(const QString &header, const QString &body)
{
    fHeader.append(header);
    fBody.append(body);
}

const QByteArray &RawMailMessage::getHeader() const
{
    return fHeader;
}

const QByteArray &RawMailMessage::getBody() const
{
    return fBody;
}
