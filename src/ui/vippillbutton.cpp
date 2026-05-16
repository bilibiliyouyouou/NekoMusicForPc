/**
 * @file vippillbutton.cpp
 * @brief 顶栏 VIP 胶囊实现
 */

#include "vippillbutton.h"

#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>

VipPillButton::VipPillButton(QWidget *parent)
    : QPushButton(QStringLiteral("VIP"), parent)
{
    setObjectName(QStringLiteral("tbVipPill"));
    setCursor(Qt::PointingHandCursor);
    setFixedHeight(28);
    setMinimumWidth(48);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    m_glow = new QGraphicsDropShadowEffect(this);
    m_glow->setOffset(0, 2);
    setGraphicsEffect(m_glow);

    m_glowAnim = new QPropertyAnimation(m_glow, "blurRadius", this);
    m_glowAnim->setDuration(2200);
    m_glowAnim->setStartValue(10.0);
    m_glowAnim->setEndValue(18.0);
    m_glowAnim->setLoopCount(-1);
    m_glowAnim->setEasingCurve(QEasingCurve::InOutSine);

    applyStyle();
}

void VipPillButton::setVipActive(bool active)
{
    if (m_active == active)
        return;
    m_active = active;
    applyStyle();
}

void VipPillButton::applyStyle()
{
    if (m_active) {
        setStyleSheet(
            QStringLiteral(
                "QPushButton#tbVipPill {"
                "  padding: 5px 14px;"
                "  font-size: 11px;"
                "  font-weight: 800;"
                "  letter-spacing: 1.2px;"
                "  color: #3d2a00;"
                "  border-radius: 14px;"
                "  border: 1px solid rgba(255, 179, 0, 0.7);"
                "  background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #ffe082, stop:1 #ffb300);"
                "}"
                "QPushButton#tbVipPill:hover {"
                "  color: #2a1d00;"
                "  background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #ffecb3, stop:1 #ffa000);"
                "}"));
        m_glow->setColor(QColor(255, 193, 7, 140));
        updateGlowAnimation();
    } else {
        setStyleSheet(
            QStringLiteral(
                "QPushButton#tbVipPill {"
                "  padding: 5px 14px;"
                "  font-size: 11px;"
                "  font-weight: 800;"
                "  letter-spacing: 1.2px;"
                "  color: #6b6b70;"
                "  border-radius: 14px;"
                "  border: 1px solid rgba(110, 110, 120, 0.35);"
                "  background: rgba(110, 110, 120, 0.22);"
                "}"
                "QPushButton#tbVipPill:hover {"
                "  color: #4a4a4f;"
                "  background: rgba(110, 110, 120, 0.32);"
                "}"));
        m_glow->setColor(QColor(0, 0, 0, 0));
        m_glow->setBlurRadius(0);
        m_glowAnim->stop();
    }
}

void VipPillButton::updateGlowAnimation()
{
    m_glowAnim->stop();
    m_glowAnim->setDirection(QAbstractAnimation::Forward);
    m_glowAnim->start();
}
