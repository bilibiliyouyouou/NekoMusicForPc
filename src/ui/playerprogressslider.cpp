#include "playerprogressslider.h"

#include "theme/thememanager.h"

#include <QEnterEvent>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QEasingCurve>

namespace {

constexpr int kGrooveH = 3;
constexpr int kHandlePx = 14;
constexpr int kAnimMs = 180;

} // namespace

PlayerProgressSlider::PlayerProgressSlider(QWidget *parent)
    : QSlider(Qt::Horizontal, parent)
{
    setMouseTracking(true);

    m_handleAnim = new QPropertyAnimation(this, "handleReveal", this);
    m_handleAnim->setDuration(kAnimMs);
    m_handleAnim->setEasingCurve(QEasingCurve::OutCubic);
}

void PlayerProgressSlider::setHandleReveal(qreal reveal)
{
    const qreal clamped = qBound(0.0, reveal, 1.0);
    if (qFuzzyCompare(m_handleReveal, clamped))
        return;
    m_handleReveal = clamped;
    update();
}

void PlayerProgressSlider::animateHandleReveal(bool show)
{
    if (!m_handleAnim)
        return;
    m_handleAnim->stop();
    m_handleAnim->setStartValue(m_handleReveal);
    m_handleAnim->setEndValue(show ? 1.0 : 0.0);
    m_handleAnim->start();
}

void PlayerProgressSlider::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const bool dark = Theme::ThemeManager::instance().isDarkMode();
    const QRect r = rect();
    const int grooveY = (r.height() - kGrooveH) / 2;
    const QRect groove(r.left(), grooveY, r.width(), kGrooveH);

    const int range = maximum() - minimum();
    const double t = range > 0 ? (value() - minimum()) / static_cast<double>(range) : 0.0;

    p.setPen(Qt::NoPen);
    p.setBrush(dark ? QColor(255, 255, 255, 16) : QColor(0, 0, 0, 26));
    p.drawRoundedRect(groove, 2, 2);

    if (t > 0.0) {
        QRect filled(groove.left(), groove.top(), qMax(kGrooveH, int(groove.width() * t)), kGrooveH);
        p.setBrush(dark ? QColor(255, 107, 139) : QColor(230, 57, 80));
        p.drawRoundedRect(filled, 2, 2);
    }

    if (m_handleReveal <= 0.001)
        return;

    const int cx = groove.left() + int(groove.width() * t);
    const int cy = r.center().y();
    const int size = qMax(2, int(kHandlePx * m_handleReveal));
    const QRect handleRect(cx - size / 2, cy - size / 2, size, size);

    p.setOpacity(m_handleReveal);
    p.setBrush(dark ? QColor(255, 183, 197) : QColor(255, 183, 197));
    p.setPen(QPen(dark ? QColor(230, 57, 80, 140) : QColor(111, 66, 193, 90), 1));
    p.drawEllipse(handleRect);
}

void PlayerProgressSlider::enterEvent(QEnterEvent *event)
{
    QSlider::enterEvent(event);
    animateHandleReveal(true);
}

void PlayerProgressSlider::leaveEvent(QEvent *event)
{
    QSlider::leaveEvent(event);
    if (!isSliderDown())
        animateHandleReveal(false);
}

void PlayerProgressSlider::mousePressEvent(QMouseEvent *event)
{
    QSlider::mousePressEvent(event);
    animateHandleReveal(true);
}

void PlayerProgressSlider::mouseReleaseEvent(QMouseEvent *event)
{
    QSlider::mouseReleaseEvent(event);
    if (!underMouse())
        animateHandleReveal(false);
}
