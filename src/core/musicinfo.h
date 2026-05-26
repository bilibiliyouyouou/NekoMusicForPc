#ifndef MUSICINFO_H
#define MUSICINFO_H

#include <QtGlobal>
#include <QString>

struct MusicInfo {
    int id = 0;
    QString title;
    QString artist;
    QString album;
    int duration = 0;
    QString coverUrl;
    /** 非空表示本机外部音频文件（绝对路径），此时 id 为负数占位，不走在线接口 */
    QString localPath;
    /** 热门榜播放量；<0 表示未设置 */
    int playCount = -1;
    /** 最新音乐上传时间（Unix 毫秒）；0 表示未设置 */
    qint64 uploadedAtMs = 0;

    bool isLocalFile() const { return !localPath.isEmpty(); }
};

#endif // MUSICINFO_H
