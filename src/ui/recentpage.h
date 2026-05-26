#pragma once

#include <QWidget>
#include <QList>
#include <QSet>

#include "core/musicinfo.h"

class QLabel;
class QPushButton;
class SongListWidget;

/** 最近播放 — 1:1 SPlayer History.vue (标题 + 播放/清空 + SongList) */
class RecentPage : public QWidget
{
    Q_OBJECT

public:
    explicit RecentPage(QWidget *parent = nullptr);

    void retranslate();
    void refresh();
    void setPlaybackPaused(bool paused);
    void updatePlayingHighlight();
    void setFavoritedMusicIds(const QSet<int> &ids);

signals:
    void playRequested(const MusicInfo &info);
    void playAllRequested(const QList<MusicInfo> &songs);
    void favoriteRequested(int musicId);
    void playPauseRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void setupUi();
    void applyPageStyle();
    void loadRecentPlays();
    void updateHeaderMeta();
    void confirmClearRecent();
    int currentPlayingMusicId() const;
    void showPageStatus(const QString &text, const char *iconName = nullptr);
    void hidePageStatus();

    QWidget *m_header = nullptr;
    QLabel *m_titleLbl = nullptr;
    QLabel *m_countLbl = nullptr;
    QPushButton *m_playBtn = nullptr;
    QPushButton *m_clearBtn = nullptr;

    SongListWidget *m_songList = nullptr;
    QWidget *m_emptyWrap = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_emptyIcon = nullptr;

    QList<MusicInfo> m_allRecent;
    QSet<int> m_favoritedIds;
};
