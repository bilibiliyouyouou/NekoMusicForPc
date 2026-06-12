/**
 * @file covercache.cpp
 * @brief 封面图片缓存实现
 *
 * Linux：进程内内存 LRU，避免 tmpfs 持续增长。
 * 其他平台：<Temp>/nekomusic-cache/covers/
 */

#include "covercache.h"
#include "theme/theme.h"

#include <QStandardPaths>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QUrl>
#include <QRegularExpression>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace {

QString safeCacheKey(const QString &value)
{
    QString key = value.trimmed();
    if (key.isEmpty())
        return {};

    const int query = key.indexOf(QLatin1Char('?'));
    if (query >= 0)
        key.truncate(query);
    const int fragment = key.indexOf(QLatin1Char('#'));
    if (fragment >= 0)
        key.truncate(fragment);

    if (key.isEmpty())
        return {};

    static const QRegularExpression safePattern(QStringLiteral("^[A-Za-z0-9._-]+$"));
    if (safePattern.match(key).hasMatch())
        return key;

    const QByteArray digest = QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha1).toHex();
    return QStringLiteral("url-") + QString::fromLatin1(digest);
}

QString resourcePathFromUrl(const QString &url)
{
    if (url.startsWith(QLatin1String("qrc:"), Qt::CaseInsensitive)) {
        const QUrl qurl(url);
        QString path = qurl.path();
        if (path.startsWith(QLatin1Char('/')))
            return QLatin1Char(':') + path;
        return QStringLiteral(":/") + path;
    }
    if (url.startsWith(QLatin1Char(':')))
        return url;
    return {};
}

} // namespace

CoverCache *CoverCache::instance()
{
    static CoverCache inst;
    return &inst;
}

CoverCache::CoverCache(QObject *parent) : QObject(parent)
{
    m_nam.setTransferTimeout(10000);
}

QString CoverCache::musicIdFromCoverUrl(const QString &coverUrl)
{
    const QString resolved = resolveCoverUrl(coverUrl);
    const QString u = resolved.isEmpty() ? coverUrl.trimmed() : resolved;
    if (u.isEmpty())
        return {};

    const QString resourcePath = resourcePathFromUrl(u);
    if (!resourcePath.isEmpty())
        return safeCacheKey(resourcePath);

    const QUrl parsed(u);
    if (parsed.isLocalFile()) {
        const QFileInfo fi(parsed.toLocalFile());
        return safeCacheKey(fi.absoluteFilePath());
    }

    QString path = parsed.isValid() && !parsed.path().isEmpty() ? parsed.path() : u;
    while (path.endsWith(QLatin1Char('/')))
        path.chop(1);
    const int slash = path.lastIndexOf(QLatin1Char('/'));
    if (slash < 0)
        return safeCacheKey(path);
    QString id = path.mid(slash + 1);
    const int q = id.indexOf(QLatin1Char('?'));
    if (q >= 0)
        id.truncate(q);
    const int fragment = id.indexOf(QLatin1Char('#'));
    if (fragment >= 0)
        id.truncate(fragment);
    static const QRegularExpression numericPattern(QStringLiteral("^[0-9]+$"));
    if (!id.isEmpty()
        && (path.contains(QStringLiteral("/api/music/cover/")) || numericPattern.match(id).hasMatch())) {
        return safeCacheKey(id);
    }
    return safeCacheKey(u);
}

QString CoverCache::resolveCoverUrl(const QString &rawUrl)
{
    const QString u = rawUrl.trimmed();
    if (u.isEmpty())
        return {};

    if (u.startsWith(QLatin1String("file:"), Qt::CaseInsensitive)
        || u.startsWith(QLatin1String("qrc:"), Qt::CaseInsensitive)
        || u.startsWith(QLatin1Char(':'))) {
        return u;
    }
    if (u.startsWith(QLatin1String("http://"), Qt::CaseInsensitive)
        || u.startsWith(QLatin1String("https://"), Qt::CaseInsensitive)) {
        return u;
    }
    if (u.startsWith(QLatin1String("//"))) {
        return QStringLiteral("https:") + u;
    }

    if (QDir::isAbsolutePath(u) && QFileInfo::exists(u))
        return QUrl::fromLocalFile(QFileInfo(u).absoluteFilePath()).toString();

    QString base = QString::fromUtf8(Theme::kApiBase);
    while (base.endsWith(QLatin1Char('/')))
        base.chop(1);
    if (u.startsWith(QLatin1Char('/')))
        return base + u;
    return base + QLatin1Char('/') + u;
}

QString CoverCache::cacheDir() const
{
#ifndef Q_OS_LINUX
    if (!m_cacheDirInitialized) {
        QString base = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        m_cacheDir = base + QStringLiteral("/nekomusic-cache/covers");
        m_cacheDirInitialized = true;
    }
    return m_cacheDir;
#else
    return {};
#endif
}

#ifdef Q_OS_LINUX

qint64 CoverCache::pixmapBytes(const QPixmap &pixmap)
{
    if (pixmap.isNull())
        return 0;
    return qint64(pixmap.width()) * pixmap.height() * 4;
}

QPixmap CoverCache::getLinuxMem(const QString &musicId) const
{
    if (musicId.isEmpty() || !m_linuxMemCache.contains(musicId))
        return {};
    m_linuxMemLru.removeAll(musicId);
    m_linuxMemLru.append(musicId);
    return m_linuxMemCache.value(musicId);
}

void CoverCache::setLinuxMem(const QString &musicId, const QPixmap &pixmap)
{
    if (musicId.isEmpty() || pixmap.isNull())
        return;

    if (m_linuxMemCache.contains(musicId)) {
        m_linuxMemBytes -= pixmapBytes(m_linuxMemCache.value(musicId));
        m_linuxMemLru.removeAll(musicId);
    }

    m_linuxMemCache.insert(musicId, pixmap);
    m_linuxMemLru.append(musicId);
    m_linuxMemBytes += pixmapBytes(pixmap);
    trimLinuxMemCache();
}

void CoverCache::trimLinuxMemCache()
{
    while (m_linuxMemBytes > kLinuxCoverMemLimitBytes && !m_linuxMemLru.isEmpty()) {
        const QString oldest = m_linuxMemLru.takeFirst();
        const QPixmap removed = m_linuxMemCache.take(oldest);
        m_linuxMemBytes -= pixmapBytes(removed);
    }
}

#endif

void CoverCache::ensureCacheDir() const
{
#ifndef Q_OS_LINUX
    QDir dir;
    dir.mkpath(cacheDir());
#endif
}

QPixmap CoverCache::get(const QString &musicId) const
{
    const QString cacheKey = safeCacheKey(musicId);
    if (cacheKey.isEmpty()) return QPixmap();
#ifdef Q_OS_LINUX
    return getLinuxMem(cacheKey);
#else
    QString path = cacheDir() + QLatin1Char('/') + cacheKey + QStringLiteral(".jpg");
    if (QFile::exists(path)) {
        QPixmap pix;
        if (pix.load(path)) return pix;
        QFile::remove(path);
    }
    return QPixmap();
#endif
}

void CoverCache::set(const QString &musicId, const QPixmap &pixmap)
{
    const QString cacheKey = safeCacheKey(musicId);
    if (cacheKey.isEmpty() || pixmap.isNull()) return;
#ifdef Q_OS_LINUX
    setLinuxMem(cacheKey, pixmap);
#else
    ensureCacheDir();
    QString path = cacheDir() + QLatin1Char('/') + cacheKey + QStringLiteral(".jpg");
    pixmap.save(path, "JPEG", 85);
#endif
}

void CoverCache::fetchCover(const QString &musicId, const QString &coverUrl)
{
    const QString absolute = resolveCoverUrl(coverUrl);
    if (absolute.isEmpty())
        return;
    const QString providedKey = safeCacheKey(musicId);
    const QString cacheKey = providedKey.isEmpty() ? musicIdFromCoverUrl(absolute) : providedKey;
    if (cacheKey.isEmpty())
        return;

    const QString resourcePath = resourcePathFromUrl(absolute);
    if (!resourcePath.isEmpty()) {
        QPixmap pix;
        if (pix.load(resourcePath)) {
            set(cacheKey, pix);
            emit coverLoaded(cacheKey, pix);
        } else {
            qWarning() << "[CoverCache] failed to load resource cover:" << absolute;
        }
        return;
    }

    if (absolute.startsWith(QLatin1String("file:"), Qt::CaseInsensitive)) {
        QPixmap pix;
        if (pix.load(QUrl(absolute).toLocalFile())) {
            set(cacheKey, pix);
            emit coverLoaded(cacheKey, pix);
        } else {
            qWarning() << "[CoverCache] failed to load local cover:" << absolute;
        }
        return;
    }

    QPixmap cached = get(cacheKey);
    if (!cached.isNull()) {
        emit coverLoaded(cacheKey, cached);
        return;
    }

    if (m_inFlight.contains(cacheKey))
        return;

    QNetworkRequest req;
    req.setUrl(QUrl(absolute));
    req.setTransferTimeout(10000);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("NekoMusic Qt"));
    req.setRawHeader("Accept", "image/png,image/jpeg,image/jpg,image/gif,image/bmp,image/svg+xml,image/*;q=0.8,*/*;q=0.5");
    QNetworkReply *reply = m_nam.get(req);
    m_inFlight.insert(cacheKey, reply);
    connect(reply, &QNetworkReply::finished, this, [this, reply, cacheKey, absolute]() {
        m_inFlight.remove(cacheKey);
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "[CoverCache] cover request failed:" << absolute << reply->errorString();
            return;
        }

        const QByteArray data = reply->readAll();
        QPixmap pix;
        if (pix.loadFromData(data)) {
            set(cacheKey, pix);
            emit coverLoaded(cacheKey, pix);
        } else {
            qWarning() << "[CoverCache] failed to decode cover:" << absolute
                       << "content-type:" << reply->header(QNetworkRequest::ContentTypeHeader).toString()
                       << "bytes:" << data.size();
        }
    });
}

void CoverCache::clear()
{
    const QList<QNetworkReply *> reps = m_inFlight.values();
    m_inFlight.clear();
    for (QNetworkReply *reply : reps) {
        if (reply) {
            reply->disconnect();
            reply->abort();
            reply->deleteLater();
        }
    }

#ifdef Q_OS_LINUX
    m_linuxMemCache.clear();
    m_linuxMemLru.clear();
    m_linuxMemBytes = 0;
#else
    ensureCacheDir();
    QDir dir(cacheDir());
    const auto entries = dir.entryInfoList({QStringLiteral("*.jpg")}, QDir::Files);
    for (const auto &entry : entries) {
        QFile::remove(entry.absoluteFilePath());
    }
#endif
}
