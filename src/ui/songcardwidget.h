#pragma once

#include <QWidget>
#include <functional>

#include "core/musicinfo.h"

class QLabel;
class QPushButton;

/** SPlayer SongCard.vue — 90px 行、序号悬停播放、封面 50、专辑/时长/收藏 */
class SongCardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SongCardWidget(QWidget *parent = nullptr);

    void bind(const MusicInfo &info, int index);
    void setPlaying(bool playing);
    void setPaused(bool paused);
    /** false = 收藏心形；true = 从歌单移除（Delete） */
    void setRemoveMode(bool remove);
    void applyTheme();

    const MusicInfo &info() const { return m_info; }

    std::function<void(const MusicInfo &)> onActivate;
    std::function<void(const MusicInfo &)> onPlayNext;
    std::function<void(int)> onUnfavorite;
    std::function<void()> onTogglePlayPause;

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    void rebuildLayout();
    void updateIndexColumn();
    void updateHoverOverlays();
    void elideTexts();
    QString formatDuration(int seconds) const;

    MusicInfo m_info;
    int m_index = 0;
    bool m_playing = false;
    bool m_paused = false;
    bool m_hover = false;
    bool m_removeMode = false;

    QWidget *m_content = nullptr;
    QWidget *m_numCol = nullptr;
    QLabel *m_indexLbl = nullptr;
    QPushButton *m_playOverlay = nullptr;
    QPushButton *m_statusOverlay = nullptr;
    QLabel *m_coverLbl = nullptr;
    QLabel *m_titleLbl = nullptr;
    QLabel *m_artistLbl = nullptr;
    QLabel *m_albumLbl = nullptr;
    QLabel *m_timeLbl = nullptr;
    QPushButton *m_heartBtn = nullptr;
};
