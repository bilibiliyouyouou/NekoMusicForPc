#include "localmusicmeta.h"

#include <QImage>
#include <QStringList>
#include <QAudioOutput>
#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QHash>
#include <QMediaMetaData>
#include <QMediaPlayer>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QStringConverter>

namespace LocalMusic {

namespace {

bool isPlaylistSuffix(const QString &suf)
{
    return suf == QLatin1String("m3u") || suf == QLatin1String("m3u8") || suf == QLatin1String("pls");
}

/** 可嵌入 M3U/PLS 的音频条目扩展名（不含列表本身） */
bool isEmbeddedAudioSuffix(const QString &suf)
{
    static const QStringList kExt = {
        QStringLiteral("mp3"),
        QStringLiteral("flac"),
        QStringLiteral("wav"),
        QStringLiteral("m4a"),
        QStringLiteral("aac"),
        QStringLiteral("ogg"),
        QStringLiteral("oga"),
        QStringLiteral("opus"),
        QStringLiteral("mp4"),
        QStringLiteral("wma"),
        QStringLiteral("mpc"),
        QStringLiteral("spx"),
        QStringLiteral("ra"),
        QStringLiteral("ram"),
    };
    return kExt.contains(suf);
}

QString firstLocalAudioFromM3u(const QString &m3uPath)
{
    QFile f(m3uPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    QTextStream in(&f);
    in.setEncoding(QStringConverter::Utf8);
    const QDir baseDir = QFileInfo(m3uPath).absoluteDir();

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;

        QString candidate;
        if (line.startsWith(QLatin1String("file:"), Qt::CaseInsensitive)) {
            QUrl u(line, QUrl::StrictMode);
            if (!u.isValid() || !u.isLocalFile())
                u = QUrl::fromUserInput(line);
            if (u.isLocalFile())
                candidate = QDir::cleanPath(u.toLocalFile());
        } else {
            QFileInfo item(line);
            if (item.isAbsolute())
                candidate = QDir::cleanPath(item.absoluteFilePath());
            else
                candidate = QDir::cleanPath(baseDir.filePath(line));
        }

        if (candidate.isEmpty())
            continue;
        QFileInfo fiCand(candidate);
        if (!fiCand.exists() || !fiCand.isFile())
            continue;
        const QString csuf = fiCand.suffix().toLower();
        if (isPlaylistSuffix(csuf))
            continue;
        if (isEmbeddedAudioSuffix(csuf))
            return candidate;
    }
    return {};
}

QString firstLocalAudioFromPls(const QString &plsPath)
{
    QFile f(plsPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    QTextStream in(&f);
    in.setEncoding(QStringConverter::Utf8);

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.size() < 6)
            continue;
        if (!line.startsWith(QLatin1String("File"), Qt::CaseInsensitive))
            continue;
        const int eq = line.indexOf(QLatin1Char('='));
        if (eq < 0)
            continue;
        const QString key = line.left(eq).trimmed();
        if (key.size() < 5)
            continue;
        // File1= ...
        QString val = line.mid(eq + 1).trimmed();
        if (val.isEmpty())
            continue;

        QString candidate;
        if (val.startsWith(QLatin1String("file:"), Qt::CaseInsensitive)) {
            QUrl u(val, QUrl::StrictMode);
            if (!u.isValid() || !u.isLocalFile())
                u = QUrl::fromUserInput(val);
            if (u.isLocalFile())
                candidate = QDir::cleanPath(u.toLocalFile());
        } else if (val.startsWith(QLatin1String("http://"), Qt::CaseInsensitive)
                   || val.startsWith(QLatin1String("https://"), Qt::CaseInsensitive)) {
            continue;
        } else {
            QFileInfo item(val);
            if (item.isAbsolute())
                candidate = QDir::cleanPath(item.absoluteFilePath());
            else
                candidate = QDir::cleanPath(QFileInfo(plsPath).absoluteDir().filePath(val));
        }

        if (candidate.isEmpty())
            continue;
        QFileInfo fiCand(candidate);
        if (!fiCand.exists() || !fiCand.isFile())
            continue;
        const QString csuf = fiCand.suffix().toLower();
        if (isPlaylistSuffix(csuf))
            continue;
        if (isEmbeddedAudioSuffix(csuf))
            return candidate;
    }
    return {};
}

} // namespace


QString normalizeOpenPathArgument(QString raw)
{
    raw = raw.trimmed();
    if (raw.size() >= 2) {
        const QChar a = raw.front();
        const QChar b = raw.back();
        if ((a == QLatin1Char('"') && b == QLatin1Char('"'))
            || (a == QLatin1Char('\'') && b == QLatin1Char('\'')))
            raw = raw.mid(1, raw.size() - 2).trimmed();
    }
    if (raw.startsWith(QLatin1String("file:"), Qt::CaseInsensitive)) {
        QUrl u = QUrl(raw, QUrl::StrictMode);
        if (!u.isValid() || !u.isLocalFile())
            u = QUrl::fromUserInput(raw);
        if (u.isLocalFile()) {
            const QString p = u.toLocalFile();
            if (!p.isEmpty())
                return QDir::cleanPath(p);
        }
        return {};
    }
    return QDir::cleanPath(raw);
}

bool isSupportedLocalAudioFile(const QString &filePath)
{
    static const QStringList kExt = {
        QStringLiteral("mp3"),
        QStringLiteral("flac"),
        QStringLiteral("wav"),
        QStringLiteral("m4a"),
        QStringLiteral("aac"),
        QStringLiteral("ogg"),
        QStringLiteral("oga"),
        QStringLiteral("opus"),
        QStringLiteral("mp4"),
        QStringLiteral("wma"),
        QStringLiteral("mpc"),
        QStringLiteral("spx"),
        QStringLiteral("ra"),
        QStringLiteral("ram"),
        QStringLiteral("m3u"),
        QStringLiteral("m3u8"),
        QStringLiteral("pls"),
    };
    const QString suf = QFileInfo(filePath).suffix().toLower();
    return kExt.contains(suf);
}

QString resolveToPlayableLocalPath(const QString &normalizedLocalPath)
{
    QFileInfo fi(normalizedLocalPath);
    if (!fi.exists() || !fi.isFile())
        return {};
    const QString suf = fi.suffix().toLower();
    if (suf == QLatin1String("m3u") || suf == QLatin1String("m3u8"))
        return firstLocalAudioFromM3u(normalizedLocalPath);
    if (suf == QLatin1String("pls"))
        return firstLocalAudioFromPls(normalizedLocalPath);
    if (isSupportedLocalAudioFile(normalizedLocalPath))
        return normalizedLocalPath;
    return {};
}

int stableLocalTrackId(const QString &canonicalOrAbsolutePath)
{
    const uint h = qHash(canonicalOrAbsolutePath);
    int id = -static_cast<int>(h & 0x7FFFFFFFu);
    if (id >= 0)
        id = -1;
    return id;
}

int onlineCatalogIdFromFilename(const QString &absoluteOrCanonicalPath)
{
    const QString base = QFileInfo(absoluteOrCanonicalPath).completeBaseName();
    if (base.isEmpty())
        return 0;
    for (QChar c : base) {
        if (c.unicode() < QLatin1Char('0').unicode() || c.unicode() > QLatin1Char('9').unicode())
            return 0;
    }
    bool ok = false;
    const int n = base.toInt(&ok);
    if (!ok || n <= 0)
        return 0;
    return n;
}

static QString resolvePath(const QString &filePath)
{
    QFileInfo fi(filePath);
    QString c = fi.canonicalFilePath();
    if (c.isEmpty())
        c = fi.absoluteFilePath();
    return c;
}

static int durationSeconds(const QMediaMetaData &md, const QMediaPlayer &player)
{
    const QVariant v = md.value(QMediaMetaData::Duration);
    if (v.isValid()) {
        bool ok = false;
        const qint64 ms = v.toLongLong(&ok);
        if (ok && ms > 0)
            return static_cast<int>(ms / 1000);
    }
    const QString ds = md.stringValue(QMediaMetaData::Duration);
    bool ok = false;
    const qint64 ms2 = ds.toLongLong(&ok);
    if (ok && ms2 > 0)
        return static_cast<int>(ms2 / 1000);
    if (player.duration() > 0)
        return static_cast<int>(player.duration() / 1000);
    return 0;
}

MusicInfo probeAndBuildInfo(const QString &filePath)
{
    MusicInfo info;
    const QString path = resolvePath(filePath);
    if (path.isEmpty())
        return info;

    info.localPath = path;
    info.id = stableLocalTrackId(path);

    QFileInfo fi(path);
    info.title = fi.completeBaseName();

    QMediaPlayer player;
    QAudioOutput audio;
    player.setAudioOutput(&audio);
    player.setSource(QUrl::fromLocalFile(path));

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(&player, &QMediaPlayer::metaDataChanged, &loop, &QEventLoop::quit);
    timer.start(5000);
    loop.exec();

    const QMediaMetaData md = player.metaData();

    const QString t = md.stringValue(QMediaMetaData::Title);
    if (!t.isEmpty())
        info.title = t;

    QString a = md.stringValue(QMediaMetaData::ContributingArtist);
    if (a.isEmpty())
        a = md.stringValue(QMediaMetaData::AlbumArtist);
    if (a.isEmpty())
        a = md.stringValue(QMediaMetaData::Author);
    if (!a.isEmpty())
        info.artist = a;

    const QString alb = md.stringValue(QMediaMetaData::AlbumTitle);
    if (!alb.isEmpty())
        info.album = alb;

    info.duration = durationSeconds(md, player);

    QImage cover;
    const QVariant cov = md.value(QMediaMetaData::CoverArtImage);
    if (cov.isValid() && cov.canConvert<QImage>())
        cover = cov.value<QImage>();
    if (cover.isNull()) {
        const QVariant th = md.value(QMediaMetaData::ThumbnailImage);
        if (th.isValid() && th.canConvert<QImage>())
            cover = th.value<QImage>();
    }
    if (!cover.isNull()) {
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
            + QStringLiteral("/nekomusic-local-art");
        QDir().mkpath(dir);
        const QString dest = dir + QLatin1Char('/') + QString::number(qHash(path))
            + QStringLiteral(".jpg");
        if (cover.save(dest, "JPEG", 90))
            info.coverUrl = QUrl::fromLocalFile(dest).toString();
    }

    return info;
}

} // namespace LocalMusic
