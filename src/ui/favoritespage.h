#pragma once

#include <QWidget>
#include <QList>

#include "core/musicinfo.h"

class QLabel;
class QPushButton;
class QLineEdit;
class RoundCoverLabel;
class QVariantAnimation;
class SongListWidget;
class ApiClient;

/** 「我喜欢的音乐」— 1:1 SPlayer liked.vue (ListDetail + SongList + SongCard) */
class FavoritesPage : public QWidget
{
    Q_OBJECT

public:
    explicit FavoritesPage(ApiClient *apiClient, QWidget *parent = nullptr);

    void retranslate();
    void refresh();
    void setPlaybackPaused(bool paused);
    void updatePlayingHighlight();

signals:
    void playRequested(int musicId, const QString &title, const QString &artist, const QString &coverUrl);
    void playAllRequested(const QList<MusicInfo> &results);
    void unfavoriteRequested(int musicId);
    void playPauseRequested();

protected:
    void paintEvent(QPaintEvent *) override;
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void setupUi();
    void applyPageStyle();
    void loadFavorites();
    void applyFilter();
    void updateHeaderMeta();
    void updateCoverImage();
    void setHeaderCompact(bool compact);
    void onListScrolled(int scrollTop);
    int currentPlayingMusicId() const;
    void showPageStatus(const QString &text, const char *iconName = nullptr);
    void hidePageStatus();

    ApiClient *m_apiClient = nullptr;

    QWidget *m_detailHeader = nullptr;
    QWidget *m_coverWrap = nullptr;
    RoundCoverLabel *m_coverImg = nullptr;
    RoundCoverLabel *m_coverShadow = nullptr;
    QWidget *m_coverMask = nullptr;
    QWidget *m_playCountRow = nullptr;
    QLabel *m_playCountLbl = nullptr;
    QLabel *m_titleLbl = nullptr;
    QWidget *m_metaRow = nullptr;
    QLabel *m_creatorLbl = nullptr;
    QPushButton *m_playBtn = nullptr;
    QPushButton *m_moreBtn = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QWidget *m_searchWrap = nullptr;

    SongListWidget *m_songList = nullptr;
    QWidget *m_emptyWrap = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_emptyIcon = nullptr;

    QVariantAnimation *m_headerAnim = nullptr;
    bool m_headerCompact = false;

    QList<MusicInfo> m_allFavorites;
    QList<MusicInfo> m_displaySongs;
    QString m_coverMusicId;
};
