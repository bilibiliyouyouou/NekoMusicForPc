#pragma once

#include <QObject>
#include <QList>

#include "core/musicinfo.h"

class ApiClient;
class QNetworkAccessManager;
class QNetworkReply;
class QFile;

/** 用户主动下载：保存到系统下载目录/NekoMusic，并记录到本地数据库 */
class MusicDownloadManager : public QObject
{
    Q_OBJECT

public:
    static MusicDownloadManager &instance();

    void setApiClient(ApiClient *apiClient);
    QString downloadDir() const;
    bool isDownloaded(int musicId) const;
    void downloadMusic(const MusicInfo &music);
    void cancelCurrent();

signals:
    void downloadProgress(int musicId, qint64 bytesReceived, qint64 bytesTotal);
    void downloadCompleted(int musicId);
    void downloadFailed(int musicId, const QString &error);
    void downloadsChanged();

private:
    explicit MusicDownloadManager(QObject *parent = nullptr);
    ~MusicDownloadManager() override;

    void startNext();
    void finishCurrent(bool success, const QString &error = {});
    void copyCachedToDownload(const MusicInfo &music, const QString &cachePath);
    void startNetworkDownload(const MusicInfo &music);
    void onNetworkProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onNetworkFinished();
    bool finalizeDownload(const MusicInfo &music, const QString &sourcePath,
                          const QString &contentType = {});
    void saveLyrics(const MusicInfo &music);

    ApiClient *m_apiClient = nullptr;
    QNetworkAccessManager *m_nam = nullptr;
    QNetworkReply *m_reply = nullptr;
    QFile *m_file = nullptr;

    QList<MusicInfo> m_queue;
    MusicInfo m_current;
    QString m_tempPath;
    bool m_busy = false;
};
