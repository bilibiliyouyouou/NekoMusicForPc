#include "defaultmusicappchecker.h"

#include <QProcess>
#include <QString>

namespace {

/** 与 packaging/nekomusic.desktop 安装名一致 */
const char kDesktopId[] = "nekomusic.desktop";

bool queryMimeDefault(const char *mime, QString *out)
{
    QProcess p;
    p.start(QStringLiteral("xdg-mime"), {QStringLiteral("query"), QStringLiteral("default"), QString::fromUtf8(mime)});
    if (!p.waitForFinished(4000) || p.exitCode() != 0)
        return false;
    *out = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
    return true;
}

bool desktopMatchesOur(const QString &desktopLine)
{
    if (desktopLine.isEmpty())
        return false;
    const QString id = QString::fromUtf8(kDesktopId);
    if (desktopLine.compare(id, Qt::CaseInsensitive) == 0)
        return true;
    return desktopLine.endsWith(QLatin1Char('/') + id, Qt::CaseInsensitive);
}

} // namespace

namespace DefaultMusicAppChecker {

bool isDefaultMusicPlayer()
{
    // 与 nekomusic.desktop 中 MimeType 一致的主要类型
    const char *mimes[] = {
        "audio/mpeg",
        "audio/flac",
        "audio/x-wav",
        "audio/ogg",
    };
    int hits = 0;
    int defined = 0;
    for (const char *mime : mimes) {
        QString def;
        if (!queryMimeDefault(mime, &def) || def.isEmpty())
            continue;
        defined++;
        if (desktopMatchesOur(def))
            hits++;
    }
    if (defined == 0)
        return true;
    // 已声明的关联里只要有一种常见音频仍不是本应用，则认为尚未完全设为默认
    return hits == defined;
}

void trySetAsDefaultMusicPlayer()
{
    QProcess::execute(QStringLiteral("xdg-mime"),
                      {QStringLiteral("default"),
                       QString::fromUtf8(kDesktopId),
                       QStringLiteral("audio/mpeg"),
                       QStringLiteral("audio/flac"),
                       QStringLiteral("audio/x-wav"),
                       QStringLiteral("audio/ogg")});
}

} // namespace DefaultMusicAppChecker
