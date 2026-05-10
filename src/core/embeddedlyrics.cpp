/**
 * @file embeddedlyrics.cpp
 * @brief 从本地音频内嵌标签读取歌词（无网络）
 */

#include "embeddedlyrics.h"

#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QString>

namespace EmbeddedLyrics {

namespace {

quint32 readBe32(const uchar *p)
{
    return (quint32(p[0]) << 24) | (quint32(p[1]) << 16) | (quint32(p[2]) << 8) | quint32(p[3]);
}

int readSynchsafe32(const uchar *p)
{
    if ((p[0] | p[1] | p[2] | p[3]) & 0x80u)
        return -1;
    return int((quint32(p[0]) << 21) | (quint32(p[1]) << 14) | (quint32(p[2]) << 7) | quint32(p[3]));
}

int readBe24(const uchar *p)
{
    return (int(p[0]) << 16) | (int(p[1]) << 8) | int(p[2]);
}

quint32 readLe32(const uchar *p)
{
    return quint32(p[0]) | (quint32(p[1]) << 8) | (quint32(p[2]) << 16) | (quint32(p[3]) << 24);
}

QString decodeUtf16WithBom(const QByteArray &b)
{
    if (b.size() < 2)
        return QString();
    const uchar c0 = uchar(b[0]);
    const uchar c1 = uchar(b[1]);
    if (c0 == 0xFF && c1 == 0xFE) {
        QString s;
        s.resize((b.size() - 2) / 2);
        QChar *d = s.data();
        for (int i = 2; i + 1 < b.size(); i += 2) {
            const auto cu = static_cast<char16_t>(uchar(b[i]) | (uchar(b[i + 1]) << 8));
            *d++ = QChar(cu);
        }
        return s;
    }
    if (c0 == 0xFE && c1 == 0xFF) {
        QString s;
        s.resize((b.size() - 2) / 2);
        QChar *d = s.data();
        for (int i = 2; i + 1 < b.size(); i += 2) {
            const auto cu = static_cast<char16_t>((uchar(b[i]) << 8) | uchar(b[i + 1]));
            *d++ = QChar(cu);
        }
        return s;
    }
    QString s;
    s.resize(b.size() / 2);
    QChar *d = s.data();
    for (int i = 0; i + 1 < b.size(); i += 2) {
        const auto cu = static_cast<char16_t>(uchar(b[i]) | (uchar(b[i + 1]) << 8));
        *d++ = QChar(cu);
    }
    return s;
}

QString decodeUtf16Be(const QByteArray &b)
{
    QString s;
    s.resize(b.size() / 2);
    QChar *d = s.data();
    for (int i = 0; i + 1 < b.size(); i += 2) {
        const auto cu = static_cast<char16_t>((uchar(b[i]) << 8) | uchar(b[i + 1]));
        *d++ = QChar(cu);
    }
    return s;
}

QString decodeId3Text(const QByteArray &b, int enc)
{
    switch (enc) {
    case 0:
        return QString::fromLatin1(b);
    case 3:
        return QString::fromUtf8(b);
    case 1:
        return decodeUtf16WithBom(b);
    case 2:
        return decodeUtf16Be(b);
    default:
        return QString::fromUtf8(b);
    }
}

/** 从 USLT / ULT 帧 body（不含帧头）解析出歌词正文 */
QString lyricsFromId3UsltBody(const QByteArray &body)
{
    if (body.size() < 1 + 3 + 1)
        return QString();
    const int enc = uchar(body[0]);
    QByteArray rest = body.mid(1 + 3); // skip language

    int lyricStart = 0;
    if (enc == 1 || enc == 2) {
        for (int i = 0; i + 1 < rest.size(); i += 2) {
            if (uchar(rest[i]) == 0 && uchar(rest[i + 1]) == 0) {
                lyricStart = i + 2;
                break;
            }
        }
        if (lyricStart == 0)
            return QString();
    } else {
        const int z = rest.indexOf('\0');
        if (z < 0)
            return QString();
        lyricStart = z + 1;
    }

    return decodeId3Text(rest.mid(lyricStart), enc).trimmed();
}

QString readId3v2Embedded(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return QString();
    const qint64 fsz = f.size();
    if (fsz < 10)
        return QString();

    QByteArray hdr = f.read(10);
    if (hdr.size() < 10 || !hdr.startsWith("ID3"))
        return QString();

    const int major = uchar(hdr[3]);
    const int flags = uchar(hdr[5]);
    const int tagBodySize = readSynchsafe32(reinterpret_cast<const uchar *>(hdr.constData() + 6));
    if (tagBodySize < 0 || tagBodySize > 50 * 1024 * 1024)
        return QString();

    QByteArray body = f.read(tagBodySize);
    if (body.size() < tagBodySize)
        return QString();

    int pos = 0;
    if (flags & 0x40) {
        if (major == 4) {
            if (body.size() < 4)
                return QString();
            const int eh = readSynchsafe32(reinterpret_cast<const uchar *>(body.constData()));
            if (eh < 4 || eh > body.size())
                return QString();
            pos = eh;
        } else if (major == 3) {
            if (body.size() < 4)
                return QString();
            const quint32 eh = readBe32(reinterpret_cast<const uchar *>(body.constData()));
            if (eh < 6 || eh > static_cast<quint32>(body.size()))
                return QString();
            pos = int(eh);
        } else {
            return QString();
        }
    }

    while (pos + 10 <= body.size()) {
        const char *id4 = body.constData() + pos;
        if (id4[0] == 0 && id4[1] == 0 && id4[2] == 0 && id4[3] == 0)
            break;

        quint32 frameSize = 0;
        int headerLen = 0;

        if (major == 2) {
            if (pos + 6 > body.size())
                break;
            const QByteArray id = body.mid(pos, 3);
            frameSize = static_cast<quint32>(readBe24(reinterpret_cast<const uchar *>(body.constData() + pos + 3)));
            headerLen = 6;
            if (frameSize > static_cast<quint32>(body.size() - pos - headerLen))
                break;

            const QByteArray frameData = body.mid(pos + headerLen, int(frameSize));
            if (id == "ULT") {
                const QString t = lyricsFromId3UsltBody(frameData);
                if (!t.isEmpty())
                    return t;
            }
            pos += headerLen + int(frameSize);
            continue;
        }

        if (major == 3) {
            if (pos + 10 > body.size())
                break;
            frameSize = readBe32(reinterpret_cast<const uchar *>(body.constData() + pos + 4));
            headerLen = 10;
        } else if (major == 4) {
            if (pos + 10 > body.size())
                break;
            const int ss = readSynchsafe32(reinterpret_cast<const uchar *>(body.constData() + pos + 4));
            if (ss < 0)
                break;
            frameSize = static_cast<quint32>(ss);
            headerLen = 10;
        } else {
            break;
        }

        if (frameSize > static_cast<quint32>(body.size() - pos - headerLen))
            break;

        const QByteArray frameId = body.mid(pos, 4);
        const QByteArray frameData = body.mid(pos + headerLen, int(frameSize));

        if (frameId == "USLT") {
            const QString t = lyricsFromId3UsltBody(frameData);
            if (!t.isEmpty())
                return t;
        }

        pos += headerLen + int(frameSize);
    }

    return QString();
}

QString readFlacVorbisLyrics(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return QString();
    if (f.read(4) != QByteArrayLiteral("fLaC"))
        return QString();

    for (;;) {
        const QByteArray hb = f.read(4);
        if (hb.size() < 4)
            return QString();
        const uchar b0 = uchar(hb[0]);
        const bool last = (b0 & 0x80u) != 0;
        const int blockType = int(b0 & 0x7Fu);
        const int blockLen = readBe24(reinterpret_cast<const uchar *>(hb.constData() + 1));
        if (blockLen < 0 || blockLen > 16 * 1024 * 1024)
            return QString();

        QByteArray block = f.read(blockLen);
        if (block.size() < blockLen)
            return QString();

        if (blockType == 4) {
            int p = 0;
            auto readLe32Str = [&](QString *out) -> bool {
                if (p + 4 > block.size())
                    return false;
                const quint32 n = readLe32(reinterpret_cast<const uchar *>(block.constData() + p));
                p += 4;
                if (n > static_cast<quint32>(block.size() - p))
                    return false;
                const QByteArray raw = block.mid(p, int(n));
                p += int(n);
                *out = QString::fromUtf8(raw);
                return true;
            };

            QString vendor;
            if (!readLe32Str(&vendor))
                return QString();
            (void)vendor;
            if (p + 4 > block.size())
                return QString();
            const quint32 count = readLe32(reinterpret_cast<const uchar *>(block.constData() + p));
            p += 4;

            QString lyricsField;
            for (quint32 i = 0; i < count && p + 4 <= block.size(); ++i) {
                const quint32 n = readLe32(reinterpret_cast<const uchar *>(block.constData() + p));
                p += 4;
                if (n > static_cast<quint32>(block.size() - p))
                    break;
                const QByteArray line = block.mid(p, int(n));
                p += int(n);
                const QString s = QString::fromUtf8(line);
                const int eq = s.indexOf(QLatin1Char('='));
                if (eq <= 0)
                    continue;
                const QString key = s.left(eq).trimmed().toUpper();
                if (key == QLatin1String("LYRICS") || key == QLatin1String("UNSYNCEDLYRICS")) {
                    const QString val = s.mid(eq + 1).trimmed();
                    if (!val.isEmpty())
                        lyricsField = val;
                }
            }
            if (!lyricsField.isEmpty())
                return lyricsField;
        }

        if (last)
            break;
    }

    return QString();
}

} // namespace

QString readEmbeddedLyricsText(const QString &localFilePath)
{
    if (localFilePath.isEmpty())
        return QString();

    const QFileInfo fi(localFilePath);
    if (!fi.isFile())
        return QString();

    QString out = readId3v2Embedded(localFilePath);
    if (!out.isEmpty())
        return out;

    out = readFlacVorbisLyrics(localFilePath);
    return out;
}

} // namespace EmbeddedLyrics
