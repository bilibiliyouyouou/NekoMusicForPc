#pragma once

#include <QString>

/** 万/亿 紧凑数字（播放量等） */
QString formatCompactCount(int n);

/** 播放量展示，如「4.8万 次播放」 */
QString formatPlayCountText(int playCount);

/** 相对上传时间，如「3天前」；uploadedAtMs 为 Unix 毫秒 */
QString formatRelativeUploadTime(qint64 uploadedAtMs);
