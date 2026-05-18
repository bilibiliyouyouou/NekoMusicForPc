/**
 * @file desktoplrc.cpp
 * @brief 桌面歌词显示实现
 */

#include "desktoplrc.h"
#include "core/i18n.h"

#include <QGuiApplication>
#include <QRegularExpression>
#include <QScreen>
#include <QSettings>

DesktopLrc::DesktopLrc(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setContextMenuPolicy(Qt::NoContextMenu);

    resize(600, 80);

    m_font = QFont(QStringLiteral("Microsoft YaHei"), 20, QFont::Bold);
    m_textColor = Qt::white;
    m_backgroundColor = QColor(0, 0, 0, 180);

    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &DesktopLrc::updateLyricDisplay);

    restoreGeometry();
    applyFallbackText();
}

DesktopLrc::~DesktopLrc() = default;

void DesktopLrc::restoreGeometry()
{
    QSettings settings;
    const bool hasPos = settings.contains(QStringLiteral("desktopLyricsX"))
        && settings.contains(QStringLiteral("desktopLyricsY"));
    if (hasPos) {
        const int x = settings.value(QStringLiteral("desktopLyricsX")).toInt();
        const int y = settings.value(QStringLiteral("desktopLyricsY")).toInt();
        const int w = settings.value(QStringLiteral("desktopLyricsW"), width()).toInt();
        const int h = settings.value(QStringLiteral("desktopLyricsH"), height()).toInt();
        resize(qMax(320, w), qMax(56, h));
        move(x, y);
        return;
    }

    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        const QRect g = screen->availableGeometry();
        move((g.width() - width()) / 2 + g.x(), g.bottom() - height() - 48);
    }
}

void DesktopLrc::saveGeometry()
{
    QSettings settings;
    settings.setValue(QStringLiteral("desktopLyricsX"), x());
    settings.setValue(QStringLiteral("desktopLyricsY"), y());
    settings.setValue(QStringLiteral("desktopLyricsW"), width());
    settings.setValue(QStringLiteral("desktopLyricsH"), height());
}

void DesktopLrc::applyFallbackText()
{
    if (!m_currentSongTitle.isEmpty() || !m_currentSongArtist.isEmpty())
        m_currentLyrics = QStringLiteral("%1 - %2").arg(m_currentSongTitle, m_currentSongArtist);
    else
        m_currentLyrics = I18n::instance().tr("noLyrics");
}

void DesktopLrc::loadLrcText(const QString &lrcText)
{
    if (lrcText.trimmed().isEmpty()) {
        m_lyricsMap.clear();
        applyFallbackText();
        update();
        return;
    }

    parseLyrics(lrcText);
    if (m_lyricsMap.isEmpty()) {
        applyFallbackText();
    } else {
        m_currentLyrics = getLyricAtTime(m_currentPosition);
    }
    update();
}

void DesktopLrc::parseLyrics(const QString &lyricsText)
{
    m_lyricsMap.clear();

    const QStringList lines = lyricsText.split(QLatin1Char('\n'));
    static const QRegularExpression timeRegex(R"(\[(\d+):(\d{1,2})(?:\.(\d{1,5}))?\])");

    auto subsecondToMs = [](const QString &frac) -> int {
        if (frac.isEmpty())
            return 0;
        bool ok = false;
        const int F = frac.toInt(&ok);
        if (!ok || F < 0)
            return 0;
        const int L = frac.length();
        if (L < 1 || L > 5)
            return 0;
        static const qint64 kPow10[] = {1, 10, 100, 1000, 10000, 100000};
        const qint64 denom = kPow10[L];
        return static_cast<int>((qint64{F} * 1000 + denom / 2) / denom);
    };

    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty())
            continue;

        QRegularExpressionMatchIterator matches = timeRegex.globalMatch(trimmed);
        if (!matches.hasNext())
            continue;

        QString lyricText = trimmed;
        lyricText.remove(timeRegex);
        lyricText = lyricText.trimmed();
        if (lyricText.isEmpty())
            continue;

        matches = timeRegex.globalMatch(trimmed);
        while (matches.hasNext()) {
            const QRegularExpressionMatch match = matches.next();
            const int minutes = match.captured(1).toInt();
            const int seconds = match.captured(2).toInt();
            const int ms = subsecondToMs(match.captured(3));
            const qint64 timeMs = static_cast<qint64>(minutes) * 60000 + seconds * 1000 + ms;
            m_lyricsMap[timeMs] = lyricText;
        }
    }
}

void DesktopLrc::updatePosition(qint64 position)
{
    m_currentPosition = position;
}

void DesktopLrc::setCurrentSong(const QString &title, const QString &artist)
{
    m_currentSongTitle = title;
    m_currentSongArtist = artist;
    m_currentPosition = 0;
    m_lyricsMap.clear();
    applyFallbackText();
    update();
}

QString DesktopLrc::getLyricAtTime(qint64 timeMs) const
{
    if (m_lyricsMap.isEmpty())
        return m_currentLyrics;

    QString lyric;
    for (auto it = m_lyricsMap.constBegin(); it != m_lyricsMap.constEnd(); ++it) {
        if (it.key() <= timeMs)
            lyric = it.value();
        else
            break;
    }

    return lyric.isEmpty() ? m_currentLyrics : lyric;
}

void DesktopLrc::updateLyricDisplay()
{
    if (!m_isVisible || !m_updateTimer->isActive())
        return;

    const QString lyric = getLyricAtTime(m_currentPosition);
    if (lyric != m_currentLyrics)
        m_currentLyrics = lyric;
    update();
}

void DesktopLrc::showWindow()
{
    m_isVisible = true;
    show();
    raise();
    m_updateTimer->start(100);
    updateLyricDisplay();
}

void DesktopLrc::hideWindow()
{
    m_isVisible = false;
    m_updateTimer->stop();
    hide();
}

void DesktopLrc::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), m_backgroundColor);

    const QString currentLyric = getLyricAtTime(m_currentPosition);
    QString nextLyric;
    qint64 nextTime = -1;

    for (auto it = m_lyricsMap.constBegin(); it != m_lyricsMap.constEnd(); ++it) {
        if (it.key() > m_currentPosition) {
            nextLyric = it.value();
            nextTime = it.key();
            break;
        }
    }

    float progress = 0.0f;
    if (nextTime > 0 && !m_lyricsMap.isEmpty()) {
        qint64 currentTime = 0;
        for (auto it = m_lyricsMap.constBegin(); it != m_lyricsMap.constEnd(); ++it) {
            if (it.key() <= m_currentPosition) {
                currentTime = it.key();
            } else {
                break;
            }
        }
        if (currentTime >= 0 && nextTime > currentTime) {
            progress = static_cast<float>(m_currentPosition - currentTime)
                       / static_cast<float>(nextTime - currentTime);
            progress = qBound(0.0f, progress, 1.0f);
        }
    }

    painter.setPen(QColor(255, 200, 0));
    painter.setFont(m_font);
    QRect lyricRect = rect();
    lyricRect.setHeight(height() / 2);
    painter.drawText(lyricRect, Qt::AlignCenter, currentLyric);

    if (!nextLyric.isEmpty()) {
        painter.setPen(QColor(255, 255, 255, 150));
        QRect nextRect = rect();
        nextRect.setTop(height() / 2);
        painter.drawText(nextRect, Qt::AlignCenter, nextLyric);
    }

    if (progress > 0.0f) {
        const int barWidth = static_cast<int>(width() * progress);
        painter.fillRect(0, height() - 3, barWidth, 3, QColor(255, 200, 0, 200));
    }

    if (m_lyricsMap.isEmpty() && !m_currentSongTitle.isEmpty()) {
        painter.setPen(m_textColor);
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 14, QFont::Normal));
        const QString songInfo =
            QStringLiteral("%1 - %2").arg(m_currentSongTitle, m_currentSongArtist);
        painter.drawText(rect(), Qt::AlignCenter | Qt::AlignBottom, songInfo);
    }
}

void DesktopLrc::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void DesktopLrc::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
}

void DesktopLrc::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        saveGeometry();
        event->accept();
    }
}
