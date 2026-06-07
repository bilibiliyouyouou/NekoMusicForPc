#pragma once

#include <QWidget>
#include <QList>
#include <QSet>

#include "core/musicinfo.h"

class QLabel;
class QPushButton;
class QStackedWidget;
class SongListWidget;

/** 下载管理 — 正在下载 / 已完成 两个分区 */
class DownloadPage : public QWidget
{
    Q_OBJECT

public:
    explicit DownloadPage(QWidget *parent = nullptr);

    void retranslate();
    void refresh();
    void setPlaybackPaused(bool paused);
    void updatePlayingHighlight();
    void setFavoritedMusicIds(const QSet<int> &ids);
    void refreshDownloadDisplay();

signals:
    void playRequested(const MusicInfo &info);
    void playAllRequested(const QList<MusicInfo> &songs);
    void favoriteRequested(int musicId);
    void playPauseRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    enum class Tab { Downloading, Completed };

    void setupUi();
    void applyPageStyle();
    void setActiveTab(Tab tab);
    void loadActiveTab();
    void loadCompletedDownloads();
    void updateHeaderMeta();
    void updateTabLabels();
    void confirmClearDownloads();
    int currentPlayingMusicId() const;
    void showPageStatus(const QString &text, const char *iconName = nullptr);
    void hidePageStatus();
    QList<MusicInfo> activeSongs() const;

    QWidget *m_header = nullptr;
    QLabel *m_titleLbl = nullptr;
    QLabel *m_countLbl = nullptr;
    QLabel *m_statusHintLbl = nullptr;
    QWidget *m_tabBar = nullptr;
    QPushButton *m_tabDownloading = nullptr;
    QPushButton *m_tabCompleted = nullptr;
    QPushButton *m_playBtn = nullptr;
    QPushButton *m_clearBtn = nullptr;

    SongListWidget *m_songList = nullptr;
    QWidget *m_emptyWrap = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_emptyIcon = nullptr;

    Tab m_activeTab = Tab::Downloading;
    QList<MusicInfo> m_pendingDownloads;
    QList<MusicInfo> m_completedDownloads;
    QSet<int> m_favoritedIds;
};
