#pragma once

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QString>

/** 从 QNetworkReply 属性推断本次响应实际使用的 HTTP 语义版本（供调试日志使用）。 */
inline QString httpProtocolLabel(const QNetworkReply *reply)
{
    if (!reply)
        return QStringLiteral("未知");

    const QVariant h2 = reply->attribute(QNetworkRequest::Http2WasUsedAttribute);
    if (h2.isValid() && h2.toBool())
        return QStringLiteral("HTTP/2");

    const QVariant h2direct = reply->attribute(QNetworkRequest::Http2DirectAttribute);
    if (h2direct.isValid() && h2direct.toBool())
        return QStringLiteral("HTTP/2");

    const QVariant pipelined = reply->attribute(QNetworkRequest::HttpPipeliningWasUsedAttribute);
    if (pipelined.isValid() && pipelined.toBool())
        return QStringLiteral("HTTP/1.1 (pipelining)");

    return QStringLiteral("HTTP/1.x");
}
