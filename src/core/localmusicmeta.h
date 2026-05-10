#pragma once

#include "musicinfo.h"
#include <QString>

namespace LocalMusic {

/**
 * 将「打开方式」/桌面 %U 传入的 file:/// URL、带引号路径等转为本地绝对路径。
 * 无法解析时返回空串。
 */
QString normalizeOpenPathArgument(QString raw);

/** 与路径绑定的稳定负数 id，用于播放列表与队列 */
int stableLocalTrackId(const QString &canonicalOrAbsolutePath);

/** 是否为本客户端支持的本地音频扩展名（mp3 / flac / wav / 播放列表等） */
bool isSupportedLocalAudioFile(const QString &filePath);

/**
 * 得到实际用于探测元数据与播放的本地路径：普通音频为自身；
 * M3U/M3U8/PLS 解析为首个可播放的本地条目（跳过仅网络地址的项）。
 */
QString resolveToPlayableLocalPath(const QString &normalizedLocalPath);

/** 阻塞式读取元数据（短时 QEventLoop），解析封面则写入临时 JPEG 并填入 coverUrl */
MusicInfo probeAndBuildInfo(const QString &filePath);

} // namespace LocalMusic
