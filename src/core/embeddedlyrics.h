#pragma once

#include <QString>

namespace EmbeddedLyrics {

/**
 * 仅从本地文件二进制内嵌标签读取歌词（不发起网络、不二次打开解码器）：
 * — ID3v2 USLT / ID3v2.2 ULT（常见于 mp3）
 * — FLAC Vorbis Comment 的 LYRICS / UNSYNCEDLYRICS
 *
 * 返回统一为 QString；无内嵌时为空。
 */
QString readEmbeddedLyricsText(const QString &localFilePath);

} // namespace EmbeddedLyrics
