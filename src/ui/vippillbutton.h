#pragma once

/**
 * @file vippillbutton.h
 * @brief 顶栏 VIP 胶囊（对齐 Web header-vip-pill / Android VipPill）
 */

#include <QPushButton>

class QGraphicsDropShadowEffect;
class QPropertyAnimation;

class VipPillButton : public QPushButton
{
    Q_OBJECT

public:
    explicit VipPillButton(QWidget *parent = nullptr);

    void setVipActive(bool active);
    bool isVipActive() const { return m_active; }

private:
    void applyStyle();
    void updateGlowAnimation();

    bool m_active = false;
    QGraphicsDropShadowEffect *m_glow = nullptr;
    QPropertyAnimation *m_glowAnim = nullptr;
};
