#pragma once

/**
 * @file addtoplaylistdialog.h
 * @brief 添加到歌单 — SPlayer $modal.create + PlaylistAdd.vue（应用内浮层）
 */

#include <QWidget>
#include <QHash>
#include <QPixmap>
#include "core/musicinfo.h"

class ApiClient;
class QScrollArea;
class QVBoxLayout;
class QLabel;
class QPushButton;
class QParallelAnimationGroup;

class AddToPlaylistDialog : public QWidget
{
    Q_OBJECT

public:
    explicit AddToPlaylistDialog(const MusicInfo &music, ApiClient *apiClient, QWidget *parent = nullptr);

    void openOn(QWidget *host);

signals:
    void playlistsChanged();
    void closed();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    struct PlaylistRow {
        int id = 0;
        QString name;
        int musicCount = 0;
        QString coverUrl;
    };

    void dismiss();
    void dismissAnimated();
    void refreshBackdrop();
    void layoutOverlay();
    void animateOpen();
    void animateClose(const std::function<void()> &onFinished);
    void stopAnimations();
    void finishOpenAnimation();
    int cardWidthForHost() const;
    int cardHeightForHost() const;
    void setupUi();
    void updateCardHeight();
    void applyTheme();
    void clearList();
    void loadPlaylists();
    void loadLocalPlaylists();
    void loadOnlinePlaylists();
    void resolveOnlineCovers(const QList<PlaylistRow> &rows);
    QWidget *appendCreateRow();
    QWidget *appendPlaylistRow(const PlaylistRow &row);
    void bindCover(QWidget *rowWidget, const QString &coverUrl);
    void openCreatePlaylist();
    void addToPlaylist(int playlistId);

    MusicInfo m_music;
    ApiClient *m_apiClient = nullptr;
    QWidget *m_host = nullptr;
    bool m_isLocal = false;
    bool m_adding = false;
    bool m_dismissing = false;

    QWidget *m_backdrop = nullptr;
    QWidget *m_card = nullptr;
    QWidget *m_cardBody = nullptr;
    QLabel *m_titleLbl = nullptr;
    QPushButton *m_closeBtn = nullptr;
    QScrollArea *m_scroll = nullptr;
    QWidget *m_listHost = nullptr;
    QVBoxLayout *m_listLay = nullptr;
    QLabel *m_emptyLbl = nullptr;
    QHash<int, QWidget *> m_rowsByPlaylistId;

    QParallelAnimationGroup *m_animGroup = nullptr;
};
