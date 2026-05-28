#pragma once

/**
 * @file homepage.h
 * @brief 首页 — 推荐歌单
 *
 * 推荐歌单：POST /api/playlists/search
 * 热门音乐：GET /api/music/ranking
 * 最新音乐：GET /api/music/latest
 */

#include <QWidget>
#include <QNetworkAccessManager>
#include <QMouseEvent>
#include <QMetaObject>
#include "playlistcard.h"
#include "core/musicinfo.h"

class GlassWidget;
class QScrollArea;
class QHBoxLayout;
class QPushButton;
class QLabel;

class HomePage : public QWidget
{
    Q_OBJECT

public:
    explicit HomePage(QWidget *parent = nullptr);

signals:
    void navigateToPlaylist(int id);
    void playMusic(int id);
    void navigateToMusicList(bool isHot);  // 导航到音乐列表页面
    void navigateToDailyRecommendations();
    void addToQueue(const MusicInfo &info);

public slots:
    void refreshData();
    void retranslate();

protected:
    void paintEvent(QPaintEvent *) override;

private:
    void setupUi();
    void refreshDailyEntry();
    void applyDailyEntryStyle();

    void fetchHotMusic();     // 热门音乐卡片
    void fetchPlaylists();    // 推荐歌单卡片
    void fetchLatestMusic();  // 最新音乐卡片
    void fetchDailyEntryCover();
    void setDailyCoverPlaceholder();
    void setDailyCoverByMusicId(int musicId);

    void rebuildRecommendSection();

    QScrollArea *m_scroll = nullptr;
    QHBoxLayout *m_cardLayout = nullptr;
    QWidget *m_cardContainer = nullptr;
    QPushButton *m_dailyEntry = nullptr;
    QLabel *m_dailyCover = nullptr;
    QLabel *m_dailyIcon = nullptr;
    QLabel *m_dailyTitle = nullptr;
    QLabel *m_dailyDesc = nullptr;
    QMetaObject::Connection m_dailyCoverConn;

    QList<PlaylistInfo> m_playlists;
    QList<MusicInfo> m_hotMusic;
    QList<MusicInfo> m_latestMusic;

    bool m_hotReady = false;
    bool m_playlistReady = false;
    bool m_latestReady = false;

    QNetworkAccessManager m_nam;
};
