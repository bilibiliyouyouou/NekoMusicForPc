#include "defaultmusicappchecker.h"

#include <QProcess>
#include <QString>
#include <QStringList>

namespace {

/** 与 packaging/nekomusic.desktop 安装名一致 */
const char kDesktopId[] = "nekomusic.desktop";

/** 与 nekomusic.desktop 中 MimeType 一致（含常见流媒体/容器别名） */
const char *kAudioMimes[] = {
    "audio/mpeg",
    "audio/flac",
    "audio/x-wav",
    "audio/ogg",
    "audio/aac",
    "audio/mp4",
    "audio/mpegurl",
    "audio/vnd.rn-realaudio",
    "audio/x-mpegurl",
    "audio/x-ms-wma",
    "audio/x-musepack",
    "audio/x-pn-realaudio",
    "audio/x-scpls",
    "audio/x-speex",
};

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
    int hits = 0;
    int defined = 0;
    for (const char *mime : kAudioMimes) {
        QString def;
        if (!queryMimeDefault(mime, &def) || def.isEmpty())
            continue;
        defined++;
        if (desktopMatchesOur(def))
            hits++;
    }
    if (defined == 0)
        return true;
    return hits == defined;
}

void trySetAsDefaultMusicPlayer()
{
    QStringList args{QStringLiteral("default"), QString::fromUtf8(kDesktopId)};
    for (const char *mime : kAudioMimes)
        args.append(QString::fromUtf8(mime));
    QProcess::execute(QStringLiteral("xdg-mime"), args);
}

} // namespace DefaultMusicAppChecker
