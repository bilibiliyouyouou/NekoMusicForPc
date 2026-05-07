#ifndef MUSICINFO_H
#define MUSICINFO_H

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

    bool isLocalFile() const { return !localPath.isEmpty(); }
};

#endif // MUSICINFO_H
