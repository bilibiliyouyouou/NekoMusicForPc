#pragma once

#include <QWidget>
#include <QList>
#include <functional>

#include "core/musicinfo.h"

class QScrollArea;
class QLabel;
class QPushButton;
class QWidget;
class SongCardWidget;

/** SPlayer SongList.vue — 虚拟列表 item-height 90、sticky 表头 */
class SongListWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SongListWidget(QWidget *parent = nullptr);

    void setSongs(const QList<MusicInfo> &songs);
    void setCurrentPlayingId(int musicId);
    void setPlaybackPaused(bool paused);
    void applyTheme();
    void retranslate();
    void scrollToTop();
    void scrollToPlaying();
    bool hasCurrentPlaying() const;
    void setRemoveMode(bool remove);

    /** 行高 90px + 行间距 12px（对齐 SPlayer SongList） */
    static constexpr int kRowHeight = 90;
    static constexpr int kRowGap = 12;
    static constexpr int kRowStride = kRowHeight + kRowGap;

    std::function<void(const MusicInfo &)> onSongActivate;
    std::function<void(const MusicInfo &)> onSongPlayNext;
    std::function<void(int)> onUnfavorite;
    std::function<void()> onTogglePlayPause;

signals:
    void scrolled(int scrollTop);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void setupUi();
    void syncContainerHeight();
    void updateVisibleRows();
    void refreshPlayingState();
    SongCardWidget *acquireCard();
    void releaseCard(SongCardWidget *card);

    static constexpr int kHeaderHeight = 40;
    static constexpr int kListPad = 0;
    static constexpr int kVisibleBuffer = 4;

    QList<MusicInfo> m_songs;
    int m_currentId = -1;
    bool m_paused = false;
    bool m_removeMode = false;

    QWidget *m_header = nullptr;
    QLabel *m_hdrNum = nullptr;
    QLabel *m_hdrTitle = nullptr;
    QLabel *m_hdrAlbum = nullptr;
    QLabel *m_hdrActions = nullptr;
    QLabel *m_hdrDuration = nullptr;

    QScrollArea *m_scroll = nullptr;
    QWidget *m_container = nullptr;

    QHash<int, SongCardWidget *> m_rowCards;
    QList<SongCardWidget *> m_cardPool;

    QPushButton *m_scrollTopBtn = nullptr;
    QPushButton *m_scrollCurrentBtn = nullptr;
};
