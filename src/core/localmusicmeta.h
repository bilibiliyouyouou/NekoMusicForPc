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

/** 是否为本客户端支持的本地音频扩展名（mp3 / flac / wav 等） */
bool isSupportedLocalAudioFile(const QString &filePath);

/** 阻塞式读取元数据（短时 QEventLoop），解析封面则写入临时 JPEG 并填入 coverUrl */
MusicInfo probeAndBuildInfo(const QString &filePath);

} // namespace LocalMusic
