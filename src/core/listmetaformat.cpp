#include "listmetaformat.h"

#include "i18n.h"

#include <QDateTime>

QString formatCompactCount(int n)
{
    if (n < 0)
        return QString();
    if (n >= 100000000)
        return QString::number(n / 100000000.0, 'f', 1) + QStringLiteral("亿");
    if (n >= 10000)
        return QString::number(n / 10000.0, 'f', 1) + QStringLiteral("万");
    return QString::number(n);
}

QString formatPlayCountText(int playCount)
{
    if (playCount < 0)
        return QStringLiteral("—");
    const QString compact = formatCompactCount(playCount);
    if (compact.isEmpty())
        return QStringLiteral("—");
    return I18n::instance().tr(QStringLiteral("playCountFmt")).arg(compact);
}

QString formatRelativeUploadTime(qint64 uploadedAtMs)
{
    if (uploadedAtMs <= 0)
        return QStringLiteral("—");

    const QDateTime uploaded = QDateTime::fromMSecsSinceEpoch(uploadedAtMs, Qt::LocalTime);
    qint64 secs = uploaded.secsTo(QDateTime::currentDateTime());
    if (secs < 0)
        secs = 0;

    auto &i18n = I18n::instance();
    if (secs < 60)
        return i18n.tr(QStringLiteral("justNow"));
    if (secs < 3600)
        return QString::number(secs / 60) + i18n.tr(QStringLiteral("minutesAgo"));
    if (secs < 86400)
        return QString::number(secs / 3600) + i18n.tr(QStringLiteral("hoursAgo"));

    const qint64 days = secs / 86400;
    if (days < 30)
        return QString::number(days) + i18n.tr(QStringLiteral("daysAgo"));
    if (days < 365)
        return QString::number(days / 30) + i18n.tr(QStringLiteral("monthsAgo"));
    return QString::number(days / 365) + i18n.tr(QStringLiteral("yearsAgo"));
}
