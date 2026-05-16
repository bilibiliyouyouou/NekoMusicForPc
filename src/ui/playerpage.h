#pragma once

#include "glasswidget.h"

#include <QWidget>
#include <QResizeEvent>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPropertyAnimation>
#include <QHash>
#include <QMetaObject>
#include "../core/musicinfo.h"
#include "../core/playerengine.h"

class ApiClient;
class QTimer;

struct LyricLine {
    qint64 time;
    QString text;
    QString translation;
};

class PlayerPage : public QWidget
{
    Q_OBJECT

public:
    explicit PlayerPage(PlayerEngine *engine, ApiClient *apiClient, QWidget *parent = nullptr);
    ~PlayerPage() override;

    void setMusicInfo(int id, const QString &title, const QString &artist,
                      const QString &album, const QString &coverUrl = QString());
    void retranslate();
    /** 在线曲走 API；本地曲读内嵌标签（ID3 USLT / FLAC 注释）再尝试同名 .lrc，不请求网络。 */
    void loadLyricsForTrack(const MusicInfo &info);
    void updateLyricHighlight(qint64 positionMs);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

signals:
    void backRequested();

private:
    void setupUi();
    void applyPlayerPageStyle();
    /** 按左栏宽度对曲名/歌手/专辑单行省略，避免过长换行堆叠溢出 */
    void applyMetaTextElide();
    void loadCover(const QString &url);
    void applyCoverPixmap(const QPixmap &sourcePixmap);
    void applyCoverUnknownLarge();
    void parseLrc(const QString &lrc);
    /** 将一段 LRC 或纯文本歌词写入 m_lyrics（无时间轴时整段作为 t=0 一行） */
    void applyLyricsRawText(const QString &raw);
    void rebuildLyricLabels();
    void resetVideoRenderState();
    void updateVideoRenderUi();
    void pollVideoRenderStatus();
    void openVideoRenderDialog();
    void downloadRenderedVideo();
    int trackDurationSec() const;

    PlayerEngine *m_engine;
    ApiClient *m_apiClient = nullptr;

    GlassWidget *m_leftGlass = nullptr;
    GlassWidget *m_rightGlass = nullptr;

    QString m_clrTitle;
    QString m_clrArtist;
    QString m_clrAlbum;
    QString m_clrLyricDim;
    QString m_clrLyricHi;
    QString m_clrLyricHiTrans;
    QString m_clrLyricHiBg;

    QPushButton *m_backBtn;
    QLabel *m_coverLabel;
    QLabel *m_titleLabel;
    QLabel *m_artistLabel;
    QLabel *m_albumLabel;
    QPushButton *m_videoRenderBtn = nullptr;
    QPushButton *m_videoDownloadBtn = nullptr;
    QLabel *m_videoStatusLbl = nullptr;
    QTimer *m_videoPollTimer = nullptr;
    QString m_fullMetaTitle;
    QString m_fullMetaArtist;
    QString m_fullMetaAlbum;
    bool m_titleIsPlaceholder = true;
    bool m_artistIsPlaceholder = true;
    QScrollArea *m_lyricsScroll;
    QWidget *m_lyricsContainer;
    QVBoxLayout *m_lyricsLayout;

    int m_musicId = 0;
    int m_trackDurationSec = 0;
    QString m_videoJobId;
    QString m_videoJobStatus;
    QString m_videoJobError;
    int m_videoRemainingToday = -1;
    bool m_videoRenderBusy = false;
    QString m_coverUrl;
    QVector<LyricLine> m_lyrics;
    /** 已解析歌词：在线为正 id；本地为 stableLocalTrackId（负） */
    QHash<int, QVector<LyricLine>> m_lyricsCache;
    QMetaObject::Connection m_coverConn;
    int m_currentLyricLine = -1;
    QPropertyAnimation *m_scrollAnim = nullptr;
    /** 每次发起/清空歌词请求自增，用于丢弃过期的异步回调（本地曲歌词 id 可与 m_musicId 不一致） */
    int m_lyricsFetchGeneration = 0;
};
