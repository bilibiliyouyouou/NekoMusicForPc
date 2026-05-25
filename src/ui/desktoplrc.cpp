/**
 * @file desktoplrc.cpp
 * @brief 桌面歌词显示实现
 */

#include "desktoplrc.h"
#include "core/i18n.h"
#include "desktoplrcplatform.h"

#include <QFont>
#include <QGuiApplication>
#include <QWindow>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QRegularExpression>
#include <QScreen>
#include <QSettings>
#include <QShowEvent>

#if defined(NEKO_HAS_LAYERSHELL_QT)
#include <LayerShellQt/Window>
#endif

namespace {

constexpr int kDefaultDesktopLyricsW = 600;
constexpr int kDefaultDesktopLyricsH = 80;
constexpr int kDesktopLyricsGeomVersion = 6;

bool isReasonableDesktopLyricsSize(int w, int h, const QScreen *screen)
{
    if (w < 320 || h < 56)
        return false;
    if (!screen)
        return w <= 1400 && h <= 400;
    const QRect area = screen->availableGeometry();
    return w <= area.width() * 3 / 4 && h <= area.height() / 3;
}

QSize normalizedDesktopLyricsSize(int w, int h, const QScreen *screen)
{
    if (isReasonableDesktopLyricsSize(w, h, screen))
        return QSize(w, h);
    return QSize(kDefaultDesktopLyricsW, kDefaultDesktopLyricsH);
}

/** 全屏 bug 会把 1920×1080 写入配置；撤销代码后仍会读出，必须校验并迁移 */
void migrateDesktopLyricsGeometry(QSettings &settings)
{
    QScreen *screen = QGuiApplication::primaryScreen();
    const int w = settings.value(QStringLiteral("desktopLyricsW"), kDefaultDesktopLyricsW).toInt();
    const int h = settings.value(QStringLiteral("desktopLyricsH"), kDefaultDesktopLyricsH).toInt();
    const int version = settings.value(QStringLiteral("desktopLyricsGeomVersion"), 0).toInt();

    if (version >= kDesktopLyricsGeomVersion && isReasonableDesktopLyricsSize(w, h, screen))
        return;

    if (!isReasonableDesktopLyricsSize(w, h, screen)) {
        settings.remove(QStringLiteral("desktopLyricsX"));
        settings.remove(QStringLiteral("desktopLyricsY"));
        settings.setValue(QStringLiteral("desktopLyricsW"), kDefaultDesktopLyricsW);
        settings.setValue(QStringLiteral("desktopLyricsH"), kDefaultDesktopLyricsH);
    }
    settings.setValue(QStringLiteral("desktopLyricsGeomVersion"), kDesktopLyricsGeomVersion);
}

QWindow *desktopLrcWindowHandle(QWidget *widget)
{
    if (!widget)
        return nullptr;
    if (QWindow *handle = widget->windowHandle())
        return handle;
    widget->createWinId();
    return widget->windowHandle();
}

} // namespace

DesktopLrc::DesktopLrc(QWidget *parent)
    : QWidget(parent, Qt::Window | Qt::Tool)
{
    setAttribute(Qt::WA_TranslucentBackground, true);

    m_layerShellActive = useLayerShellPath();
    desktopLrcConfigureWindow(this, m_layerShellActive);

    setWindowTitle(QStringLiteral("NekoMusic — %1")
                 .arg(I18n::instance().tr(QStringLiteral("desktopLyrics"))));
    resize(600, 80);

    m_font = QFont(QStringLiteral("Microsoft YaHei"), 20, QFont::Bold);
    m_textColor = Qt::white;
    m_backgroundColor = QColor(0, 0, 0, 180);

    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &DesktopLrc::updateLyricDisplay);

    m_stayOnTopTimer = new QTimer(this);
    m_stayOnTopTimer->setInterval(800);
    connect(m_stayOnTopTimer, &QTimer::timeout, this, &DesktopLrc::refreshStayOnTop);

    if (!m_layerShellActive)
        restoreGeometry();
    applyFallbackText();
}

bool DesktopLrc::useLayerShellPath() const
{
#if defined(NEKO_HAS_LAYERSHELL_QT)
    return desktopLrcIsKdePlasmaSession() && desktopLrcIsWaylandSession();
#else
    Q_UNUSED(this);
    return false;
#endif
}

#if defined(NEKO_HAS_LAYERSHELL_QT)
void DesktopLrc::ensureLayerShellConfigured()
{
    if (m_layerShellConfigured)
        return;

    // 必须在首次 show/map 之前绑定 layer-shell，否则会与 xdg_toplevel 冲突导致 Wayland 崩溃
    if (LayerShellQt::Window *layer = LayerShellQt::Window::get(desktopLrcWindowHandle(this))) {
        using LSWindow = LayerShellQt::Window;
        layer->setLayer(LSWindow::LayerOverlay);
        layer->setKeyboardInteractivity(LSWindow::KeyboardInteractivityNone);
        layer->setActivateOnShow(false);
        layer->setCloseOnDismissed(false);
        layer->setScope(QStringLiteral("nekomusic-desktop-lrc"));
        layer->setAnchors(LSWindow::Anchors(LSWindow::AnchorBottom | LSWindow::AnchorLeft));
        layer->setDesiredSize(size());
    }

    m_layerShellConfigured = true;
}

void DesktopLrc::applyLayerShellGeometry()
{
    LayerShellQt::Window *layer = LayerShellQt::Window::get(desktopLrcWindowHandle(this));
    if (!layer)
        return;

    QSettings settings;
    migrateDesktopLyricsGeometry(settings);

    QScreen *screen = QGuiApplication::primaryScreen();
    const QSize sz = normalizedDesktopLyricsSize(
        settings.value(QStringLiteral("desktopLyricsW"), kDefaultDesktopLyricsW).toInt(),
        settings.value(QStringLiteral("desktopLyricsH"), kDefaultDesktopLyricsH).toInt(),
        screen);
    resize(sz);
    layer->setDesiredSize(size());
    if (!screen)
        return;

    const QRect area = screen->availableGeometry();
    int left = (area.width() - width()) / 2 + area.left();
    int top = area.bottom() - height() - 48;

    if (settings.contains(QStringLiteral("desktopLyricsX"))
        && settings.contains(QStringLiteral("desktopLyricsY"))) {
        left = settings.value(QStringLiteral("desktopLyricsX")).toInt();
        top = settings.value(QStringLiteral("desktopLyricsY")).toInt();
    }

    left = qBound(area.left(), left, area.right() - width() + 1);
    top = qBound(area.top(), top, area.bottom() - height() + 1);

    const int bottom = area.bottom() - top - height() + 1;
    layer->setAnchors(
        LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorBottom | LayerShellQt::Window::AnchorLeft));
    layer->setMargins(QMargins(left - area.left(), 0, 0, qMax(0, bottom)));
}

void DesktopLrc::saveLayerShellGeometry()
{
    LayerShellQt::Window *layer = LayerShellQt::Window::get(desktopLrcWindowHandle(this));
    if (!layer)
        return;

    QScreen *screen = QGuiApplication::screenAt(geometry().center());
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    if (!screen || !isReasonableDesktopLyricsSize(width(), height(), screen))
        return;

    const QRect area = screen->availableGeometry();
    const QMargins m = layer->margins();
    const int left = area.left() + m.left();
    const int top = area.bottom() - m.bottom() - height() + 1;

    QSettings settings;
    settings.setValue(QStringLiteral("desktopLyricsX"), left);
    settings.setValue(QStringLiteral("desktopLyricsY"), top);
    settings.setValue(QStringLiteral("desktopLyricsW"), width());
    settings.setValue(QStringLiteral("desktopLyricsH"), height());
    settings.setValue(QStringLiteral("desktopLyricsGeomVersion"), kDesktopLyricsGeomVersion);
}
#else
void DesktopLrc::ensureLayerShellConfigured() {}
void DesktopLrc::applyLayerShellGeometry() {}
void DesktopLrc::saveLayerShellGeometry() {}
#endif

void DesktopLrc::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (!m_layerShellActive)
        desktopLrcKeepOnTop(this);
}

void DesktopLrc::refreshStayOnTop()
{
    if (!m_isVisible || m_layerShellActive)
        return;
    desktopLrcKeepOnTop(this);
}

DesktopLrc::~DesktopLrc() = default;

void DesktopLrc::restoreGeometry()
{
    QSettings settings;
    migrateDesktopLyricsGeometry(settings);

    QScreen *primary = QGuiApplication::primaryScreen();
    const QSize sz = normalizedDesktopLyricsSize(
        settings.value(QStringLiteral("desktopLyricsW"), kDefaultDesktopLyricsW).toInt(),
        settings.value(QStringLiteral("desktopLyricsH"), kDefaultDesktopLyricsH).toInt(),
        primary);
    resize(sz);

    const bool hasPos = settings.contains(QStringLiteral("desktopLyricsX"))
        && settings.contains(QStringLiteral("desktopLyricsY"));

    auto placeDefault = [this]() {
        if (QScreen *screen = QGuiApplication::primaryScreen()) {
            const QRect g = screen->availableGeometry();
            move((g.width() - width()) / 2 + g.x(), g.bottom() - height() - 48);
        }
    };

    if (!hasPos) {
        placeDefault();
        return;
    }

    const int x = settings.value(QStringLiteral("desktopLyricsX")).toInt();
    const int y = settings.value(QStringLiteral("desktopLyricsY")).toInt();
    move(x, y);

    const QRect frame = geometry();
    for (QScreen *screen : QGuiApplication::screens()) {
        if (screen && screen->availableGeometry().intersects(frame))
            return;
    }
    placeDefault();
}

void DesktopLrc::saveGeometry()
{
    QScreen *screen = QGuiApplication::screenAt(geometry().center());
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    if (!screen || !isReasonableDesktopLyricsSize(width(), height(), screen))
        return;

    QSettings settings;
    settings.setValue(QStringLiteral("desktopLyricsX"), x());
    settings.setValue(QStringLiteral("desktopLyricsY"), y());
    settings.setValue(QStringLiteral("desktopLyricsW"), width());
    settings.setValue(QStringLiteral("desktopLyricsH"), height());
    settings.setValue(QStringLiteral("desktopLyricsGeomVersion"), kDesktopLyricsGeomVersion);
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

    if (m_layerShellActive) {
        ensureLayerShellConfigured();
        applyLayerShellGeometry();
        show();
        m_updateTimer->start(100);
        updateLyricDisplay();
        return;
    }

    restoreGeometry();
    show();
    desktopLrcKeepOnTop(this);
    m_updateTimer->start(100);
    m_stayOnTopTimer->start();
    updateLyricDisplay();
}

void DesktopLrc::hideWindow()
{
    m_isVisible = false;
    m_updateTimer->stop();
    m_stayOnTopTimer->stop();
    hide();
}

void DesktopLrc::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(0, 0, width(), height(), m_backgroundColor);

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
            if (it.key() <= m_currentPosition)
                currentTime = it.key();
            else
                break;
        }
        if (currentTime >= 0 && nextTime > currentTime) {
            progress = static_cast<float>(m_currentPosition - currentTime)
                       / static_cast<float>(nextTime - currentTime);
            progress = qBound(0.0f, progress, 1.0f);
        }
    }

    painter.setPen(QColor(255, 200, 0));
    painter.setFont(m_font);
    QRect lyricRect(0, 0, width(), height() / 2);
    painter.drawText(lyricRect, Qt::AlignCenter, currentLyric);

    if (!nextLyric.isEmpty()) {
        painter.setPen(QColor(255, 255, 255, 150));
        QRect nextRect(0, height() / 2, width(), height() / 2);
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
        painter.drawText(QRect(0, 0, width(), height()), Qt::AlignCenter | Qt::AlignBottom, songInfo);
    }
}

void DesktopLrc::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragPosition = event->position().toPoint();
        event->accept();
    }
}

void DesktopLrc::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_dragging || !(event->buttons() & Qt::LeftButton))
        return;

#if defined(NEKO_HAS_LAYERSHELL_QT)
    if (m_layerShellActive) {
        LayerShellQt::Window *layer = LayerShellQt::Window::get(desktopLrcWindowHandle(this));
        QScreen *screen = QGuiApplication::screenAt(event->globalPosition().toPoint());
        if (layer && screen) {
            const QRect area = screen->availableGeometry();
            const QPoint topLeft = (event->globalPosition() - QPointF(m_dragPosition)).toPoint();
            const int left = qBound(area.left(), topLeft.x(), area.right() - width() + 1);
            const int top = qBound(area.top(), topLeft.y(), area.bottom() - height() + 1);
            const int bottom = area.bottom() - top - height() + 1;
            layer->setAnchors(
                LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorBottom
                                              | LayerShellQt::Window::AnchorLeft));
            layer->setMargins(QMargins(left - area.left(), 0, 0, qMax(0, bottom)));
        }
        event->accept();
        return;
    }
#endif

    move((event->globalPosition() - QPointF(m_dragPosition)).toPoint());
    event->accept();
}

void DesktopLrc::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        if (m_layerShellActive)
            saveLayerShellGeometry();
        else
            saveGeometry();
        event->accept();
    }
}
