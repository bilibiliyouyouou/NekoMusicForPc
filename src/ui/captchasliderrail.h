#pragma once

/**
 * @file captchasliderrail.h
 * @brief 与 Web 注册页一致的宽轨道滑条（高 44px、拇指 48×36，行程与拼图 offset 线性映射）
 */

#include <QWidget>

class CaptchaSliderRail final : public QWidget
{
    Q_OBJECT

public:
    explicit CaptchaSliderRail(QWidget *parent = nullptr);

    /** 背景图宽度与拼图块宽度（与后端 challenge 一致） */
    void setChallenge(int bgWidth, int pieceWidth);
    int offset() const { return m_offset; }
    void setOffset(int x);
    void setInteractive(bool on);
    void setVerifying(bool on);

signals:
    void offsetChanged(int x);
    void interactionReleased();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    int thumbMaxTravel() const;
    int maxOffset() const;
    int thumbLeftForOffset(int off) const;
    int offsetForThumbLeft(int thumbLeft) const;
    void applyThumbLeft(int thumbLeft);

    int m_bgWidth = 300;
    int m_pieceW = 52;
    int m_offset = 0;
    bool m_interactive = false;
    bool m_verifying = false;
    bool m_dragging = false;
    int m_dragDx = 0;
};
