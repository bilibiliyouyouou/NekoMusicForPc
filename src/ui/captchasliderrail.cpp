/**
 * @file captchasliderrail.cpp
 */

#include "captchasliderrail.h"
#include "theme/thememanager.h"

#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>

namespace {
constexpr int kRailH = 44;
constexpr int kThumbW = 48;
constexpr int kThumbH = 36;
constexpr int kThumbTop = 4;
constexpr int kTrackInset = 12;
constexpr int kTrackH = 8;
constexpr int kRadius = 22;
constexpr int kThumbRadius = 10;
} // namespace

CaptchaSliderRail::CaptchaSliderRail(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(kRailH);
    setCursor(Qt::PointingHandCursor);
    setMouseTracking(false);
}

void CaptchaSliderRail::setChallenge(int bgWidth, int pieceWidth)
{
    m_bgWidth = qMax(1, bgWidth);
    m_pieceW = qMax(1, pieceWidth);
    setFixedWidth(m_bgWidth);
    m_offset = 0;
    update();
}

int CaptchaSliderRail::thumbMaxTravel() const
{
    return qMax(0, m_bgWidth - kThumbW);
}

int CaptchaSliderRail::maxOffset() const
{
    return qMax(0, m_bgWidth - m_pieceW);
}

int CaptchaSliderRail::thumbLeftForOffset(int off) const
{
    const int mx = maxOffset();
    const int tm = thumbMaxTravel();
    if (mx <= 0 || tm <= 0)
        return 0;
    const int clamped = qBound(0, off, mx);
    return qRound(double(clamped) / double(mx) * double(tm));
}

int CaptchaSliderRail::offsetForThumbLeft(int thumbLeft) const
{
    const int mx = maxOffset();
    const int tm = thumbMaxTravel();
    if (mx <= 0)
        return 0;
    if (tm <= 0)
        return 0;
    const int clamped = qBound(0, thumbLeft, tm);
    return qRound(double(clamped) / double(tm) * double(mx));
}

void CaptchaSliderRail::setOffset(int x)
{
    const int mx = maxOffset();
    const int v = qBound(0, x, mx);
    if (v == m_offset)
        return;
    m_offset = v;
    update();
    emit offsetChanged(m_offset);
}

void CaptchaSliderRail::setInteractive(bool on)
{
    m_interactive = on;
    if (!on) {
        m_dragging = false;
        releaseMouse();
    }
    update();
}

void CaptchaSliderRail::setVerifying(bool on)
{
    m_verifying = on;
    if (on)
        setCursor(Qt::ArrowCursor);
    else
        setCursor(Qt::PointingHandCursor);
    update();
}

void CaptchaSliderRail::applyThumbLeft(int thumbLeft)
{
    setOffset(offsetForThumbLeft(thumbLeft));
}

void CaptchaSliderRail::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const bool dark = Theme::ThemeManager::instance().isDarkMode();
    const QRectF railRect(0, 0, width(), height());

    // 轨道背景（对齐 Web .slider-rail）
    QColor bgTop(255, 255, 255, dark ? 35 : 140);
    QColor border(106, 90, 205, dark ? 90 : 56);
    QPen pen(border);
    pen.setWidthF(1);
    p.setPen(pen);
    p.setBrush(bgTop);
    p.drawRoundedRect(railRect.adjusted(0.5, 0.5, -0.5, -0.5), kRadius, kRadius);

    if (m_verifying) {
        p.setPen(QPen(QColor(106, 90, 205, 140), 2));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(railRect.adjusted(1, 1, -1, -1), kRadius - 1, kRadius - 1);
    }

    // 内凹槽条 .slider-rail-track-line
    const qreal trackY = (height() - kTrackH) / 2.0;
    QRectF track(kTrackInset, trackY, width() - 2 * kTrackInset, kTrackH);
    QLinearGradient g(track.left(), 0, track.right(), 0);
    if (dark) {
        g.setColorAt(0, QColor(80, 78, 98));
        g.setColorAt(1, QColor(58, 56, 74));
    } else {
        g.setColorAt(0, QColor(224, 224, 232));
        g.setColorAt(1, QColor(200, 200, 216));
    }
    p.setPen(Qt::NoPen);
    p.setBrush(g);
    p.drawRoundedRect(track, 4, 4);

    // 拇指 .slider-rail-thumb
    const int tl = thumbLeftForOffset(m_offset);
    QRectF thumb(tl, kThumbTop, kThumbW, kThumbH);
    QLinearGradient tg(thumb.topLeft(), thumb.bottomRight());
    if (m_verifying || !m_interactive) {
        tg.setColorAt(0, QColor(120, 115, 150));
        tg.setColorAt(1, QColor(90, 88, 115));
    } else {
        tg.setColorAt(0, QColor(106, 90, 205));
        tg.setColorAt(1, QColor(155, 125, 212));
    }
    p.setBrush(tg);
    p.setPen(QPen(QColor(255, 255, 255, 35), 1));
    p.drawRoundedRect(thumb.adjusted(0.5, 0.5, -0.5, -0.5), kThumbRadius, kThumbRadius);

    p.setPen(QPen(Qt::white, 1));
    p.setBrush(Qt::NoBrush);
    QFont f = p.font();
    f.setBold(true);
    f.setPointSizeF(f.pointSizeF() + 1.2);
    p.setFont(f);
    p.drawText(thumb, Qt::AlignCenter, QStringLiteral("››"));
}

void CaptchaSliderRail::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !m_interactive || m_verifying)
        return;

    const int x = event->pos().x();
    const int tl = thumbLeftForOffset(m_offset);
    if (x >= tl && x < tl + kThumbW) {
        m_dragging = true;
        m_dragDx = x - tl;
    } else {
        const int thumbLeft = qBound(0, x, thumbMaxTravel());
        applyThumbLeft(thumbLeft);
        m_dragging = true;
        m_dragDx = x - thumbLeftForOffset(m_offset);
    }
    grabMouse();
    event->accept();
}

void CaptchaSliderRail::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_dragging || !m_interactive || m_verifying)
        return;
    const int x = event->pos().x();
    const int thumbLeft = qBound(0, x - m_dragDx, thumbMaxTravel());
    applyThumbLeft(thumbLeft);
    event->accept();
}

void CaptchaSliderRail::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;
    if (m_dragging) {
        m_dragging = false;
        releaseMouse();
        if (m_interactive && !m_verifying)
            emit interactionReleased();
    }
    event->accept();
}
