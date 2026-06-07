#pragma once

/**
 * @file musiclistpage.h
 * @brief 热门 / 最新音乐列表页（SongList + SongCard 虚拟列表）
 */

#include <QWidget>
#include <QList>
#include <QSet>

#include "core/musicinfo.h"

class QLabel;
class QPushButton;
class ApiClient;
class SongListWidget;

class MusicListPage : public QWidget
{
    Q_OBJECT

public:
    enum Type { Hot, Latest, Daily };

    explicit MusicListPage(Type type, QWidget *parent = nullptr);

signals:
    void playMusic(const MusicInfo &info);
    void playAllRequested(const QList<MusicInfo> &results);
    void addToQueue(const MusicInfo &info);
    void addToPlaylist(const MusicInfo &info);
    void downloadRequested(const MusicInfo &info);
    void favoriteRequested(int musicId);
    void playPauseRequested();
    void backRequested();

public slots:
    void refresh();
    void releaseCachedData();
    void retranslate();
    void setPlaybackPaused(bool paused);
    void setFavoritedMusicIds(const QSet<int> &ids);
    void updatePlayingHighlight();

protected:
    void showEvent(QShowEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void setupUi();
    void applyPageStyle();
    void fetchData();
    void showLoadingState();
    void showPageStatus(const QString &text, const char *iconName = nullptr);
    void hidePageStatus();
    void presentSongs();
    void showSongContextMenu(const MusicInfo &info, const QPoint &globalPos);
    int currentPlayingMusicId() const;
    void updateHeaderMeta();
    void backfillDurationsFromMusicInfo(int gen);
    QString pageTitle() const;
    QString pageDesc() const;

    Type m_type;
    ApiClient *m_api = nullptr;

    QWidget *m_header = nullptr;
    QPushButton *m_backBtn = nullptr;
    QLabel *m_titleLbl = nullptr;
    QLabel *m_descLbl = nullptr;
    QLabel *m_countLbl = nullptr;
    QPushButton *m_playAllBtn = nullptr;

    SongListWidget *m_songList = nullptr;
    QWidget *m_emptyWrap = nullptr;
    QLabel *m_emptyIcon = nullptr;
    QLabel *m_statusLabel = nullptr;

    QList<MusicInfo> m_musicList;
    QSet<int> m_favoritedIds;
    int m_fetchGeneration = 0;
    bool m_durationBackfillScheduled = false;
};
