#pragma once

/**
 * @file playlistpanel.h
 * @brief 播放列表面板 — 浮动在播放器右侧
 *
 * 显示当前播放队列，支持点击播放、移除、清空操作。
 * 列表采用可见区虚拟化，大列表打开不卡顿，样式与原先一致。
 */

#include <QWidget>
#include <QList>
#include <QHash>

class QScrollArea;
class QVBoxLayout;
class QLabel;
class QPushButton;

class PlaylistPanel : public QWidget {
    Q_OBJECT

public:
    explicit PlaylistPanel(QWidget *parent = nullptr);

signals:
    void playRequested(int musicId);
    void hideRequested();

public slots:
    void refresh();
    void retranslate();
    void showPanel();
    void hidePanel();
    void togglePanel();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void setupUi();
    void applyPanelChrome();
    void syncContainerHeight();
    void updateVisibleRows();
    void clearAllCards();
    void updateCountLabel();

    static constexpr int kRowHeight = 64;
    static constexpr int kRowSpacing = 6;
    static constexpr int kRowStride = kRowHeight + kRowSpacing;
    static constexpr int kVisibleBuffer = 4;

    QLabel *m_titleLabel = nullptr;
    QLabel *m_countLabel = nullptr;
    QPushButton *m_clearBtn = nullptr;
    QPushButton *m_closeBtn = nullptr;
    QScrollArea *m_scroll = nullptr;
    QWidget *m_listContainer = nullptr;
    QLabel *m_emptyLabel = nullptr;
    QWidget *m_divider = nullptr;
    QHash<int, QWidget *> m_rowCards;
    QList<QWidget *> m_cardPool;
    int m_lastPlaylistSize = 0;
};
