#pragma once

/**
 * @file playlistpanel.h
 * @brief 播放队列抽屉 — 从窗口右侧滑入，盖住底栏播放器（对齐 SPlayer n-drawer）
 *
 * 显示当前播放队列，支持点击播放、移除、清空操作。
 * 列表采用可见区虚拟化，大列表打开不卡顿。
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

    static constexpr int kDrawerWidth = 400;

    bool isDrawerOpen() const { return m_drawerOpen; }
    /** 宿主尺寸变化时保持贴右（打开中则保持展开，关闭则保持屏外） */
    void syncToHost();

signals:
    void playRequested(int musicId);
    void hideRequested();
    void drawerClosed();

public slots:
    void refresh();
    void retranslate();
    void openDrawer();
    void closeDrawer();
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
    void scrollToCurrent();
    void updateFooterButtonIcons();

    /** SPlayer VirtualScroll item-height 80（行高 64 + 上下间距 8+8） */
    static constexpr int kRowHeight = 64;
    static constexpr int kRowSpacing = 16;
    static constexpr int kRowStride = kRowHeight + kRowSpacing;
    static constexpr int kVisibleBuffer = 4;

    QLabel *m_titleLabel = nullptr;
    QLabel *m_countLabel = nullptr;
    QPushButton *m_clearBtn = nullptr;
    QPushButton *m_scrollCurrentBtn = nullptr;
    QPushButton *m_closeBtn = nullptr;
    QScrollArea *m_scroll = nullptr;
    QWidget *m_listContainer = nullptr;
    QLabel *m_emptyLabel = nullptr;
    QWidget *m_footer = nullptr;
    QHash<int, QWidget *> m_rowCards;
    QList<QWidget *> m_cardPool;
    int m_lastPlaylistSize = 0;
    bool m_drawerOpen = false;
    bool m_animating = false;
    class QPropertyAnimation *m_slideAnim = nullptr;
};
