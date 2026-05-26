#include "songcardwidget.h"
#include "roundcoverlabel.h"

#include "core/covercache.h"
#include "theme/theme.h"
#include "theme/thememanager.h"
#include "ui/svgicon.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QFontMetrics>
#include <QApplication>
#include <QTimer>

namespace {

constexpr int kRowH = 90;
constexpr int kCover = 50;
constexpr QColor kPrimary(230, 57, 80);

/** SPlayer SongCard .song-content — surface / play / hover 边框 */
QColor songFill(bool dark, bool playing)
{
    if (playing)
        return QColor(kPrimary.red(), kPrimary.green(), kPrimary.blue(), 71); // rgba(primary, 0.28)
    return dark ? QColor(0x24, 0x24, 0x24) : QColor(Qt::white);               // surface-container
}

QColor songBorder(bool playing, bool hover)
{
    if (playing || hover)
        return QColor(kPrimary.red(), kPrimary.green(), kPrimary.blue(), 148); // rgba(primary, 0.58)
    return QColor(kPrimary.red(), kPrimary.green(), kPrimary.blue(), 31);       // rgba(primary, 0.12)
}

} // namespace

SongCardWidget::SongCardWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("SongCardWidget"));
    setFixedHeight(kRowH);
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_StyledBackground, false);
    rebuildLayout();
}

void SongCardWidget::rebuildLayout()
{
    if (m_content)
        return;

    auto *outer = new QHBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    m_content = new QWidget(this);
    m_content->setObjectName(QStringLiteral("songCardContent"));
    auto *lay = new QHBoxLayout(m_content);
    lay->setContentsMargins(12, 8, 12, 8);
    lay->setSpacing(0);

    m_numCol = new QWidget(m_content);
    m_numCol->setFixedWidth(40);

    m_indexLbl = new QLabel(m_numCol);
    m_indexLbl->setGeometry(0, 0, 40, 74);
    m_indexLbl->setAlignment(Qt::AlignCenter);
    m_indexLbl->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    m_playOverlay = new QPushButton(m_numCol);
    m_playOverlay->setFlat(true);
    m_playOverlay->setFixedSize(28, 28);
    m_playOverlay->setCursor(Qt::PointingHandCursor);
    m_playOverlay->hide();
    connect(m_playOverlay, &QPushButton::clicked, this, [this](bool) {
        if (onPlayNext)
            onPlayNext(m_info);
    });

    m_statusOverlay = new QPushButton(m_numCol);
    m_statusOverlay->setFlat(true);
    m_statusOverlay->setFixedSize(28, 28);
    m_statusOverlay->setCursor(Qt::PointingHandCursor);
    m_statusOverlay->hide();
    connect(m_statusOverlay, &QPushButton::clicked, this, [this](bool) {
        if (onTogglePlayPause)
            onTogglePlayPause();
    });

    lay->addWidget(m_numCol);
    lay->addSpacing(12);

    auto *titleCol = new QWidget(m_content);
    auto *titleLay = new QHBoxLayout(titleCol);
    titleLay->setContentsMargins(0, 4, 20, 4);
    titleLay->setSpacing(0);

    m_coverLbl = new RoundCoverLabel(8, titleCol);
    m_coverLbl->setFixedSize(kCover, kCover);
    titleLay->addWidget(m_coverLbl);
    titleLay->addSpacing(12);

    auto *infoCol = new QWidget(titleCol);
    auto *infoLay = new QVBoxLayout(infoCol);
    infoLay->setContentsMargins(0, 0, 0, 0);
    infoLay->setSpacing(2);
    m_titleLbl = new QLabel(infoCol);
    m_artistLbl = new QLabel(infoCol);
    infoLay->addWidget(m_titleLbl);
    infoLay->addWidget(m_artistLbl);
    titleLay->addWidget(infoCol, 1);

    lay->addWidget(titleCol, 1);

    m_albumLbl = new QLabel(m_content);
    m_albumLbl->setMinimumWidth(80);
    lay->addWidget(m_albumLbl, 1);

    m_heartBtn = new QPushButton(m_content);
    m_heartBtn->setFixedSize(40, 40);
    m_heartBtn->setFlat(true);
    m_heartBtn->setCursor(Qt::PointingHandCursor);
    connect(m_heartBtn, &QPushButton::clicked, this, [this](bool) {
        if (onUnfavorite)
            onUnfavorite(m_info.id);
    });
    lay->addWidget(m_heartBtn);

    m_timeLbl = new QLabel(m_content);
    m_timeLbl->setFixedWidth(50);
    m_timeLbl->setAlignment(Qt::AlignCenter);
    lay->addWidget(m_timeLbl);

    outer->addWidget(m_content, 1);

    for (QLabel *lbl : {m_indexLbl, m_coverLbl, m_titleLbl, m_artistLbl, m_albumLbl, m_timeLbl})
        lbl->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    installContentEventFilters();
    applyTheme();
}

void SongCardWidget::installContentEventFilters()
{
    if (!m_content)
        return;

    std::function<void(QWidget *)> walk = [&](QWidget *w) {
        if (!w || isInteractiveButton(w))
            return;
        w->installEventFilter(this);
        for (QObject *child : w->children()) {
            if (auto *cw = qobject_cast<QWidget *>(child))
                walk(cw);
        }
    };
    walk(m_content);
}

bool SongCardWidget::isInteractiveButton(QObject *obj) const
{
    return obj == m_heartBtn || obj == m_playOverlay || obj == m_statusOverlay;
}

void SongCardWidget::setHover(bool hover)
{
    if (m_hover == hover)
        return;
    m_hover = hover;
    updateHoverOverlays();
    update();
}

void SongCardWidget::syncHoverFromCursor()
{
    QWidget *under = QApplication::widgetAt(mapToGlobal(rect().center()));
    const bool inside = under && (under == this || isAncestorOf(under));
    setHover(inside);
}

bool SongCardWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (isInteractiveButton(watched))
        return QWidget::eventFilter(watched, event);

    switch (event->type()) {
    case QEvent::Enter:
        setHover(true);
        break;
    case QEvent::Leave:
        QTimer::singleShot(0, this, [this]() { syncHoverFromCursor(); });
        break;
    case QEvent::MouseButtonDblClick: {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton && onActivate) {
            onActivate(m_info);
            return true;
        }
        break;
    }
    default:
        break;
    }
    return QWidget::eventFilter(watched, event);
}

void SongCardWidget::bind(const MusicInfo &info, int index)
{
    m_info = info;
    m_index = index;
    m_titleLbl->setText(info.title);
    m_artistLbl->setText(info.artist);
    m_albumLbl->setText(info.album.isEmpty() ? QStringLiteral("—") : info.album);
    m_timeLbl->setText(formatDuration(info.duration));

    const QString musicId = QString::number(info.id);
    if (QPixmap cached = CoverCache::instance()->get(musicId); !cached.isNull()) {
        m_coverLbl->setPixmap(cached);
        m_coverLbl->update();
    } else {
        QPixmap pm(kCover, kCover);
        pm.fill(QColor(230, 57, 80, 40));
        m_coverLbl->setPixmap(pm);
        m_coverLbl->update();
        connect(CoverCache::instance(), &CoverCache::coverLoaded, this,
                [this, musicId](const QString &id, const QPixmap &pix) {
                    if (id == musicId) {
                        m_coverLbl->setPixmap(pix);
                        m_coverLbl->update();
                    }
                });
        const QString url = info.coverUrl.isEmpty()
            ? QString::fromUtf8("%1/api/music/cover/%2").arg(Theme::kApiBase).arg(info.id)
            : info.coverUrl;
        CoverCache::instance()->fetchCover(musicId, url);
    }

    updateIndexColumn();
    elideTexts();
    applyTheme();
}

void SongCardWidget::setPlaying(bool playing)
{
    if (m_playing == playing)
        return;
    m_playing = playing;
    updateIndexColumn();
    updateHoverOverlays();
    update();
}

void SongCardWidget::setPaused(bool paused)
{
    if (m_paused == paused)
        return;
    m_paused = paused;
    updateHoverOverlays();
    update();
}

void SongCardWidget::setRemoveMode(bool remove)
{
    if (m_removeMode == remove)
        return;
    m_removeMode = remove;
    applyTheme();
}

void SongCardWidget::applyTheme()
{
    const bool dark = Theme::ThemeManager::instance().isDarkMode();
    const QString titleFg = dark ? QString::fromUtf8(Theme::kTextMain) : QStringLiteral("#212529");
    const QString subFg = dark ? QString::fromUtf8(Theme::kTextSub) : QStringLiteral("rgba(33,37,41,0.62)");
    const QString idxFg = dark ? QString::fromUtf8(Theme::kTextMuted) : QStringLiteral("rgba(33,37,41,0.45)");

    m_indexLbl->setStyleSheet(QStringLiteral(
        "QLabel { color: %1; font-size: 14px; font-weight: 700; }").arg(idxFg));
    m_titleLbl->setStyleSheet(QStringLiteral(
        "QLabel { color: %1; font-size: 16px; font-weight: 500; }").arg(titleFg));
    m_artistLbl->setStyleSheet(QStringLiteral(
        "QLabel { color: %1; font-size: 13px; opacity: 0.6; }").arg(subFg));
    m_albumLbl->setStyleSheet(QStringLiteral(
        "QLabel { color: %1; font-size: 13px; }").arg(subFg));
    m_timeLbl->setStyleSheet(QStringLiteral(
        "QLabel { color: %1; font-size: 13px; }").arg(subFg));

    if (m_removeMode) {
        const QColor delIc = dark ? QColor(244, 246, 255, 200) : QColor(33, 37, 41, 200);
        m_heartBtn->setIcon(Icons::renderNamed("Delete", 20, delIc));
    } else {
        m_heartBtn->setIcon(Icons::renderNamed("Favorite", 20, QColor(255, 69, 69)));
    }
    m_heartBtn->setIconSize(QSize(20, 20));
    m_heartBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: transparent; border: none; border-radius: 8px; }"
        "QPushButton:hover { background: rgba(230,57,80,0.15); }"));

    const QColor overlayIc = kPrimary;
    m_playOverlay->setIcon(Icons::renderNamed("Play", 28, overlayIc));
    m_playOverlay->setIconSize(QSize(28, 28));
    m_statusOverlay->setIcon(Icons::renderNamed(m_paused ? "Play" : "Pause", 28, overlayIc));
    m_statusOverlay->setIconSize(QSize(28, 28));
    m_playOverlay->setStyleSheet(QStringLiteral("QPushButton { background: transparent; border: none; }"));
    m_statusOverlay->setStyleSheet(QStringLiteral("QPushButton { background: transparent; border: none; }"));

    update();
}

void SongCardWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    QRectF body = QRectF(m_content->geometry());
    if (body.isEmpty())
        body = QRectF(0, 0, width(), height());

    const QRectF r = body.adjusted(0.5, 0.5, -0.5, -0.5);
    QPainterPath path;
    path.addRoundedRect(r, 12, 12);

    const bool dark = Theme::ThemeManager::instance().isDarkMode();
    p.fillPath(path, songFill(dark, m_playing));
    p.setPen(QPen(songBorder(m_playing, m_hover), 2));
    p.drawPath(path);
}

void SongCardWidget::enterEvent(QEnterEvent *e)
{
    m_hover = true;
    updateHoverOverlays();
    update();
    QWidget::enterEvent(e);
}

void SongCardWidget::leaveEvent(QEvent *e)
{
    m_hover = false;
    updateHoverOverlays();
    update();
    QWidget::leaveEvent(e);
}

void SongCardWidget::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    elideTexts();
}

void SongCardWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && onActivate)
        onActivate(m_info);
    QWidget::mouseDoubleClickEvent(event);
}

void SongCardWidget::updateIndexColumn()
{
    if (m_playing) {
        m_indexLbl->setPixmap(Icons::renderNamed("Music", 22, kPrimary));
        m_indexLbl->setText(QString());
    } else {
        m_indexLbl->setPixmap(QPixmap());
        m_indexLbl->setText(QString::number(m_index + 1));
    }
    updateHoverOverlays();
}

void SongCardWidget::updateHoverOverlays()
{
    const bool showPlay = m_hover && !m_playing;
    const bool showStatus = m_hover && m_playing;

    m_indexLbl->setVisible(!showPlay && !showStatus);
    m_playOverlay->setVisible(showPlay);
    m_statusOverlay->setVisible(showStatus);

    if (showStatus) {
        m_statusOverlay->setIcon(Icons::renderNamed(m_paused ? "Play" : "Pause", 28, kPrimary));
    }

    if (m_numCol) {
        QPoint center((m_numCol->width() - 28) / 2, (m_numCol->height() - 28) / 2);
        m_playOverlay->move(center);
        m_statusOverlay->move(center);
    }
}

void SongCardWidget::elideTexts()
{
    if (!m_titleLbl || !m_artistLbl || !m_albumLbl)
        return;
    const QFontMetrics tf(m_titleLbl->font());
    const QFontMetrics af(m_artistLbl->font());
    const QFontMetrics alf(m_albumLbl->font());
    m_titleLbl->setText(tf.elidedText(m_info.title, Qt::ElideRight, qMax(40, m_titleLbl->width())));
    m_artistLbl->setText(af.elidedText(m_info.artist, Qt::ElideRight, qMax(40, m_artistLbl->width())));
    m_albumLbl->setText(alf.elidedText(
        m_info.album.isEmpty() ? QStringLiteral("—") : m_info.album, Qt::ElideRight,
        qMax(60, m_albumLbl->width())));
}

QString SongCardWidget::formatDuration(int seconds) const
{
    if (seconds <= 0)
        return QStringLiteral("--:--");
    return QStringLiteral("%1:%2")
        .arg(seconds / 60, 2, 10, QChar('0'))
        .arg(seconds % 60, 2, 10, QChar('0'));
}
