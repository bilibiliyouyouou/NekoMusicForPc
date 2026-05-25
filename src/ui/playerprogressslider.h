#pragma once

#include <QSlider>

class QPropertyAnimation;

/** 底栏进度条：细轨道常驻，滑块圆点悬停/拖动时淡入缩放（对齐 SPlayer） */
class PlayerProgressSlider : public QSlider
{
    Q_OBJECT
    Q_PROPERTY(qreal handleReveal READ handleReveal WRITE setHandleReveal)

public:
    explicit PlayerProgressSlider(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void setHandleReveal(qreal reveal);
    qreal handleReveal() const { return m_handleReveal; }
    void animateHandleReveal(bool show);

    qreal m_handleReveal = 0.0;
    QPropertyAnimation *m_handleAnim = nullptr;
};
