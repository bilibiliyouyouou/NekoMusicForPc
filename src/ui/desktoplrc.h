#pragma once

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QMap>
#include <QTimer>

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
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void parseLyrics(const QString &lyricsText);
    QString getLyricAtTime(qint64 timeMs) const;
    void updateLyricDisplay();
    void restoreGeometry();
    void saveGeometry();
    void applyFallbackText();

    QString m_currentLyrics;
    bool m_dragging = false;
    QPoint m_dragPosition;

    QMap<qint64, QString> m_lyricsMap;
    qint64 m_currentPosition = 0;
    QString m_currentSongTitle;
    QString m_currentSongArtist;

    QFont m_font;
    QColor m_textColor;
    QColor m_backgroundColor;
    bool m_isVisible = false;

    QTimer *m_updateTimer = nullptr;
};
