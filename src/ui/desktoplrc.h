#pragma once

#include <QWidget>
#include <QColor>
#include <QFont>
#include <QMap>
#include <QPoint>
#include <QTimer>

class QPaintEvent;
class QMouseEvent;
class QShowEvent;
class QScreen;

class DesktopLrc : public QWidget
{
    Q_OBJECT

public:
    explicit DesktopLrc(QWidget *parent = nullptr);
    ~DesktopLrc() override;

    void loadLrcText(const QString &lrcText);
    void updatePosition(qint64 position);
    void setCurrentSong(const QString &title, const QString &artist);

public slots:
    void showWindow();
    void hideWindow();

protected:
    void showEvent(QShowEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    bool useLayerShellPath() const;
    void ensureLayerShellConfigured();
    void applyLayerShellGeometry();
    void saveLayerShellGeometry();

    void parseLyrics(const QString &lyricsText);
    QString getLyricAtTime(qint64 timeMs) const;
    void updateLyricDisplay();
    void restoreGeometry();
    void saveGeometry();
    void applyFallbackText();
    void refreshStayOnTop();
    QScreen *screenForWidget() const;
    void setWindowScreenTopLeft(const QPoint &topLeft, QScreen *screen = nullptr);
    void syncCachedScreenPosFromSettings(QScreen *screen);
    QPoint clampToScreen(const QPoint &topLeft, QScreen *screen) const;

    QString m_currentLyrics;
    bool m_dragging = false;
    QPoint m_lastDragGlobal;
    QPoint m_cachedScreenPos;
    QPoint m_dragPressGlobal;
    QPoint m_dragAnchorPos;
    QPoint m_dragVisualOffset;
    QScreen *m_dragScreen = nullptr;

    QMap<qint64, QString> m_lyricsMap;
    qint64 m_currentPosition = 0;
    QString m_currentSongTitle;
    QString m_currentSongArtist;

    QFont m_font;
    QColor m_textColor;
    QColor m_backgroundColor;
    bool m_isVisible = false;

    bool m_layerShellActive = false;
    bool m_layerShellConfigured = false;

    QTimer *m_updateTimer = nullptr;
    QTimer *m_stayOnTopTimer = nullptr;
};
