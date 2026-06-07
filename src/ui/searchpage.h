#pragma once

/**
 * @file searchpage.h
 * @brief 搜索页 — 1:1 SPlayer Search/layout.vue + songs/playlists/artists
 */

#include <QWidget>
#include <QList>
#include <QSet>
#include <QVariantMap>

#include "core/musicinfo.h"

class QLabel;
class QPushButton;
class QStackedWidget;
class QScrollArea;
class QWidget;
class QGridLayout;
class ApiClient;
class SongListWidget;
class CoverListCard;
class CoverGridHost;

class SearchPage : public QWidget
{
    Q_OBJECT

public:
    explicit SearchPage(ApiClient *apiClient, QWidget *parent = nullptr);

    void search(const QString &query);
    void retranslate();
    void setFavoritedMusicIds(const QSet<int> &ids);
    void setPlaybackPaused(bool paused);
    void updatePlayingHighlight();
    void refreshDownloadDisplay();

signals:
    void playMusic(const MusicInfo &info);
    void downloadRequested(const MusicInfo &info);
    void downloadAllRequested(const QList<MusicInfo> &songs);
    void openPlaylist(int playlistId);
    void openArtist(const QVariantMap &artist);
    void favoriteRequested(int musicId);
    void playPauseRequested();

protected:
    void showEvent(QShowEvent *event) override;

private:
    enum Tab { Songs = 0, Playlists = 1, Artists = 2 };

    void setupUi();
    void applyPageStyle();
    void setActiveTab(Tab tab);
    void updateTitleRow();
    void showSongsEmpty(const QString &text);
    void hideSongsEmpty();
    void showPlaylistEmpty(const QString &text);
    void hidePlaylistEmpty();
    void showArtistEmpty(const QString &text);
    void hideArtistEmpty();
    void showHintState(bool hint);

    void fetchMusicResults(bool append);
    void fetchPlaylistResults();
    void fetchArtistResults();
    void applyMusicResults();
    void applyPlaylistResults();
    void relayoutPlaylistGrid();
    void applyArtistResults();

    void onSongListScrolled(int scrollTop);
    int currentPlayingMusicId() const;

    static MusicInfo musicFromMap(const QVariantMap &item);

    ApiClient *m_apiClient = nullptr;

    QLabel *m_keywordLbl = nullptr;
    QLabel *m_suffixLbl = nullptr;
    QPushButton *m_tabSongs = nullptr;
    QPushButton *m_tabPlaylists = nullptr;
    QPushButton *m_tabArtists = nullptr;
    QStackedWidget *m_tabStack = nullptr;

    QWidget *m_songsActionRow = nullptr;
    QPushButton *m_songsPlayAllBtn = nullptr;
    QPushButton *m_songsDownloadAllBtn = nullptr;
    SongListWidget *m_songList = nullptr;
    QWidget *m_songsEmptyWrap = nullptr;
    QLabel *m_songsEmptyIcon = nullptr;
    QLabel *m_songsEmptyLbl = nullptr;
    QPushButton *m_loadMoreBtn = nullptr;

    QScrollArea *m_playlistScroll = nullptr;
    CoverGridHost *m_playlistGridHost = nullptr;
    QWidget *m_playlistGridInner = nullptr;
    QList<CoverListCard *> m_playlistCards;
    QWidget *m_playlistEmptyWrap = nullptr;
    QLabel *m_playlistEmptyIcon = nullptr;
    QLabel *m_playlistEmptyLbl = nullptr;

    QScrollArea *m_artistGridScroll = nullptr;
    QWidget *m_artistGridHost = nullptr;
    QGridLayout *m_artistGrid = nullptr;
    QWidget *m_artistEmptyWrap = nullptr;
    QLabel *m_artistEmptyIcon = nullptr;
    QLabel *m_artistEmptyLbl = nullptr;

    QWidget *m_hintWrap = nullptr;
    QLabel *m_hintLbl = nullptr;

    QString m_query;
    Tab m_activeTab = Songs;
    QList<MusicInfo> m_musicResults;
    QList<QVariantMap> m_playlistResults;
    QList<QVariantMap> m_artistResults;
    QSet<int> m_favoritedIds;

    int m_musicPage = 1;
    int m_musicTotal = 0;
    bool m_musicLoading = false;
    bool m_musicHasMore = false;
    bool m_playlistLoading = false;
    bool m_artistLoading = false;

    static constexpr int kMusicPageSize = 50;
};
