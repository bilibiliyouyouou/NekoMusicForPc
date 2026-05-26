#pragma once

/**
 * @file artistdetailpage.h
 * @brief 歌手详情子页 — 搜索艺术家结果点击进入
 */

#include <QWidget>
#include <QList>
#include <QSet>
#include <QVariantMap>

#include "core/musicinfo.h"

class QLabel;
class QPushButton;
class SongListWidget;

class ArtistDetailPage : public QWidget
{
    Q_OBJECT

public:
    explicit ArtistDetailPage(QWidget *parent = nullptr);

    void loadArtist(const QVariantMap &artist);
    void retranslate();
    void setFavoritedMusicIds(const QSet<int> &ids);
    void setPlaybackPaused(bool paused);
    void updatePlayingHighlight();

signals:
    void playMusic(const MusicInfo &info);
    void playAllRequested(const QList<MusicInfo> &songs);
    void favoriteRequested(int musicId);
    void playPauseRequested();
    void backRequested();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void setupUi();
    void applyPageStyle();
    void updateHeaderMeta();
    int currentPlayingMusicId() const;
    static QList<MusicInfo> tracksFromArtistMap(const QVariantMap &artist);

    QWidget *m_topBar = nullptr;
    QPushButton *m_backBtn = nullptr;
    QWidget *m_header = nullptr;
    QLabel *m_titleLbl = nullptr;
    QLabel *m_countLbl = nullptr;
    QPushButton *m_playBtn = nullptr;
    SongListWidget *m_songList = nullptr;

    QString m_artistName;
    QList<MusicInfo> m_tracks;
    QSet<int> m_favoritedIds;
};
