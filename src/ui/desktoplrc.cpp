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

constexpr int kDefaultDesktopLyricsW = 380;
constexpr int kDefaultDesktopLyricsH = 64;
constexpr int kDesktopLyricsGeomVersion = 6;

bool isReasonableDesktopLyricsSize(int w, int h, const QScreen *screen)
{
    if (w < 200 || h < 40)
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
    // 悬浮歌词：显示时不抢主窗口焦点，更像桌面挂件
    setAttribute(Qt::WA_ShowWithoutActivating, true);

    m_layerShellActive = useLayerShellPath();
    desktopLrcConfigureWindow(this, m_layerShellActive);

    setWindowTitle(QStringLiteral("NekoMusic — %1")
                 .arg(I18n::instance().tr(QStringLiteral("desktopLyrics"))));
    resize(380, 64);

    m_font = QFont(QStringLiteral("Microsoft YaHei"), 12, QFont::Bold);
    m_textColor = Qt::white;
    m_backgroundColor = QColor(25, 25, 25, 185);

    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &DesktopLrc::updateLyricDisplay);

    m_stayOnTopTimer = new QTimer(this);
    m_stayOnTopTimer->setInterval(350);
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
        layer->setWantsToBeOnActiveScreen(false);
        layer->setAnchors(LSWindow::Anchors(LSWindow::AnchorTop | LSWindow::AnchorLeft));
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
    if (settings.contains(QStringLiteral("desktopLyricsX"))
        && settings.contains(QStringLiteral("desktopLyricsY"))) {
        const int probeW = settings.value(QStringLiteral("desktopLyricsW"), kDefaultDesktopLyricsW).toInt();
        const int probeH = settings.value(QStringLiteral("desktopLyricsH"), kDefaultDesktopLyricsH).toInt();
        const QPoint saved(settings.value(QStringLiteral("desktopLyricsX")).toInt(),
                           settings.value(QStringLiteral("desktopLyricsY")).toInt());
        if (QScreen *savedScreen =
                QGuiApplication::screenAt(saved + QPoint(probeW / 2, probeH / 2)))
            screen = savedScreen;
    }
    if (!screen)
        screen = screenForWidget();

    const QSize sz = normalizedDesktopLyricsSize(
        settings.value(QStringLiteral("desktopLyricsW"), kDefaultDesktopLyricsW).toInt(),
        settings.value(QStringLiteral("desktopLyricsH"), kDefaultDesktopLyricsH).toInt(),
        screen);
    resize(sz);
    layer->setDesiredSize(size());
    if (!screen)
        return;

    syncCachedScreenPosFromSettings(screen);
    setWindowScreenTopLeft(m_cachedScreenPos, screen);
}

void DesktopLrc::syncCachedScreenPosFromSettings(QScreen *screen)
{
    if (!screen)
        screen = screenForWidget();
    if (!screen)
        return;

    QSettings settings;
    migrateDesktopLyricsGeometry(settings);
    const QRect area = screen->availableGeometry();

    if (settings.contains(QStringLiteral("desktopLyricsX"))
        && settings.contains(QStringLiteral("desktopLyricsY"))) {
        m_cachedScreenPos = QPoint(settings.value(QStringLiteral("desktopLyricsX")).toInt(),
                                   settings.value(QStringLiteral("desktopLyricsY")).toInt());
    } else {
        m_cachedScreenPos = QPoint((area.width() - width()) / 2 + area.left(),
                                   area.bottom() - height() - 48);
    }
}

void DesktopLrc::saveLayerShellGeometry()
{
    QSettings settings;
    settings.setValue(QStringLiteral("desktopLyricsX"), m_cachedScreenPos.x());
    settings.setValue(QStringLiteral("desktopLyricsY"), m_cachedScreenPos.y());
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

void DesktopLrc::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange) {
        if (isMinimized() || isMaximized()) {
            // Force block minimization and maximization, restore to normal geometry
            QTimer::singleShot(0, this, [this]() {
                showNormal();
                if (!m_layerShellActive) {
                    desktopLrcKeepOnTop(this);
                }
            });
            event->accept();
            return;
        }
    }
    QWidget::changeEvent(event);
}

void DesktopLrc::mouseDoubleClickEvent(QMouseEvent *event)
{
    // Swallow double click event entirely to block double click to maximize
    event->accept();
}

void DesktopLrc::refreshStayOnTop()
{
    if (!m_isVisible || m_layerShellActive || m_dragging)
        return;
    desktopLrcKeepOnTop(this);
}

QScreen *DesktopLrc::screenForWidget() const
{
    auto *self = const_cast<DesktopLrc *>(this);
    if (QWindow *handle = desktopLrcWindowHandle(self)) {
        if (QScreen *screen = handle->screen())
            return screen;
    }
    if (QScreen *screen = QGuiApplication::screenAt(m_cachedScreenPos + QPoint(width() / 2, height() / 2)))
        return screen;
    return QGuiApplication::primaryScreen();
}

QPoint DesktopLrc::clampToScreen(const QPoint &topLeft, QScreen *screen) const
{
    if (!screen)
        return topLeft;
    const QRect area = screen->availableGeometry();
    return QPoint(qBound(area.left(), topLeft.x(), area.right() - width() + 1),
                  qBound(area.top(), topLeft.y(), area.bottom() - height() + 1));
}

void DesktopLrc::setWindowScreenTopLeft(const QPoint &topLeft, QScreen *screen)
{
    if (!screen)
        screen = screenForWidget();
    if (!screen)
        return;

    m_cachedScreenPos = clampToScreen(topLeft, screen);
    const QRect area = screen->availableGeometry();
    const int left = m_cachedScreenPos.x();
    const int top = m_cachedScreenPos.y();

#if defined(NEKO_HAS_LAYERSHELL_QT)
    if (m_layerShellActive) {
        if (LayerShellQt::Window *layer = LayerShellQt::Window::get(desktopLrcWindowHandle(this))) {
            layer->setWantsToBeOnActiveScreen(false);
            layer->setScreen(screen);
            layer->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorTop
                                                           | LayerShellQt::Window::AnchorLeft));
            layer->setMargins(QMargins(left - area.left(), top - area.top(), 0, 0));
            return;
        }
    }
#endif
    move(left, top);
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
            setWindowScreenTopLeft(
                QPoint((g.width() - width()) / 2 + g.x(), g.bottom() - height() - 48), screen);
        }
    };

    if (!hasPos) {
        placeDefault();
        return;
    }

    const int x = settings.value(QStringLiteral("desktopLyricsX")).toInt();
    const int y = settings.value(QStringLiteral("desktopLyricsY")).toInt();
    setWindowScreenTopLeft(QPoint(x, y), primary);

    const QRect frame = QRect(m_cachedScreenPos, size());
    for (QScreen *screen : QGuiApplication::screens()) {
        if (screen && screen->availableGeometry().intersects(frame))
            return;
    }
    placeDefault();
}

void DesktopLrc::saveGeometry()
{
    QScreen *screen = m_dragScreen ? m_dragScreen : screenForWidget();
    if (!screen || !isReasonableDesktopLyricsSize(width(), height(), screen))
        return;

    QSettings settings;
    settings.setValue(QStringLiteral("desktopLyricsX"), m_cachedScreenPos.x());
    settings.setValue(QStringLiteral("desktopLyricsY"), m_cachedScreenPos.y());
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
    update();
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
    if (isMinimized() || isMaximized())
        showNormal();
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
    if (m_layerShellActive && m_dragging && !m_dragVisualOffset.isNull())
        painter.translate(m_dragVisualOffset);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw beautiful rounded capsule background (e.g. radius 16px)
    painter.setPen(Qt::NoPen);
    painter.setBrush(m_backgroundColor);
    painter.drawRoundedRect(rect(), 16, 16);

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

    // Let's use single-line or tight dual-line layout. 
    // Since the window is compact (380x64), a neat single lyric line in the center, or micro dual-line works best.
    // If nextLyric is empty, center the current lyric vertically.
    // Let's draw current lyric with bright color (e.g., gold/yellow or white) and next lyric slightly faded.
    painter.setFont(m_font);

    // Padding inside the capsule
    int sidePadding = 20;
    QRect textRect = rect().adjusted(sidePadding, 4, -sidePadding, -4);

    if (nextLyric.isEmpty()) {
        // Centered single lyric
        painter.setPen(QColor(255, 200, 0));
        painter.drawText(textRect, Qt::AlignCenter, currentLyric);
    } else {
        // Dual line micro layout
        painter.setPen(QColor(255, 200, 0));
        QRect lyricRect(sidePadding, 4, width() - 2 * sidePadding, height() / 2 - 2);
        painter.drawText(lyricRect, Qt::AlignCenter, currentLyric);

        painter.setPen(QColor(255, 255, 255, 150));
        painter.setFont(QFont(m_font.family(), m_font.pointSize() - 2, QFont::Medium));
        QRect nextRect(sidePadding, height() / 2 - 2, width() - 2 * sidePadding, height() / 2 - 2);
        painter.drawText(nextRect, Qt::AlignCenter, nextLyric);
    }

    // Draw progress bar inside the capsule bottom border
    if (progress > 0.0f) {
        int barH = 3;
        int barW = static_cast<int>((width() - 32) * progress);
        painter.setBrush(QColor(255, 200, 0, 220));
        painter.drawRoundedRect(16, height() - 6, barW, barH, 1.5, 1.5);
    }

    if (m_lyricsMap.isEmpty() && !m_currentSongTitle.isEmpty()) {
        painter.setPen(m_textColor);
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 10, QFont::Normal));
        const QString songInfo =
            QStringLiteral("%1 - %2").arg(m_currentSongTitle, m_currentSongArtist);
        painter.drawText(textRect, Qt::AlignCenter, songInfo);
    }
}

void DesktopLrc::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragPressGlobal = event->globalPosition().toPoint();
        m_lastDragGlobal = m_dragPressGlobal;
        m_dragAnchorPos = m_cachedScreenPos;
        m_dragVisualOffset = QPoint();
        m_dragScreen = QGuiApplication::screenAt(m_dragPressGlobal);
        if (!m_dragScreen)
            m_dragScreen = screenForWidget();

        if (m_updateTimer)
            m_updateTimer->stop();

        if (!m_layerShellActive && desktopLrcIsWaylandSession()) {
            if (QWindow *handle = window() ? window()->windowHandle() : nullptr)
                handle->startSystemMove();
        }

        event->accept();
    }
}

void DesktopLrc::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_dragging || !(event->buttons() & Qt::LeftButton))
        return;

    const QPoint global = event->globalPosition().toPoint();

#if defined(NEKO_HAS_LAYERSHELL_QT)
    if (m_layerShellActive) {
        // compositor 拖动时延迟应用 margins，用绘制偏移提供实时反馈
        m_dragVisualOffset = global - m_dragPressGlobal;
        update();
        event->accept();
        return;
    }
#endif

    const QPoint delta = global - m_lastDragGlobal;
    if (delta.isNull())
        return;
    m_lastDragGlobal = global;

    if (!desktopLrcIsWaylandSession())
        setWindowScreenTopLeft(m_cachedScreenPos + delta, m_dragScreen);
    event->accept();
}

void DesktopLrc::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
#if defined(NEKO_HAS_LAYERSHELL_QT)
        if (m_layerShellActive) {
            const QPoint finalPos = clampToScreen(m_dragAnchorPos + m_dragVisualOffset, m_dragScreen);
            setWindowScreenTopLeft(finalPos, m_dragScreen);
        }
#endif
        m_dragging = false;
        m_dragVisualOffset = QPoint();
        m_dragScreen = nullptr;
        if (m_layerShellActive)
            saveLayerShellGeometry();
        else
            saveGeometry();
        if (m_updateTimer && m_isVisible)
            m_updateTimer->start(100);
        update();
        event->accept();
    }
}
