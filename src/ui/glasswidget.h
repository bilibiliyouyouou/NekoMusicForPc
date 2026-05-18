#pragma once

/**
 * @file glasswidget.h
 * @brief 液态玻璃容器：底层 QPainter 抓取背后内容 + 圆角磨砂叠色（不依赖 OpenGL，Wayland 可用）。
 */

#include <QColor>
#include <QWidget>

class GlassPlate;

class GlassWidget : public QWidget
{
    Q_OBJECT
    friend class GlassPlate;

public:
    explicit GlassWidget(QWidget *parent = nullptr);
    ~GlassWidget() override;

    qreal opacity() const;
    void setOpacity(qreal v);

    QColor baseColor() const;
    void setBaseColor(const QColor &c);

    QColor borderColor() const;
    void setBorderColor(const QColor &c);

    int borderRadius() const;
    void setBorderRadius(int r);

    /** 背后内容变化时（滚动、切页、主题）调用以重新 grab */
    void refreshBackdrop();

    /** 将子控件布局在此层之上（勿对 GlassWidget 再 setLayout，否则会顶掉底层玻璃绘制层） */
    QWidget *contentWidget() const { return m_content; }

    /**
     * 从指定控件抓取「背后」像素（例如底部播放栏 parent 为自身时，应设为顶层窗口以取到主内容区）。
     * nullptr 表示使用 parentWidget()。
     */
    void setBackdropSource(QWidget *source) { m_backdropSource = source; }

    /** 为 false 时不抓取背后像素，仅用 baseColor 实心填充（亮色卡片置于深色底上时使用） */
    void setBackdropCaptureEnabled(bool enabled);
    bool backdropCaptureEnabled() const { return m_backdropCaptureEnabled; }

private:
    void captureBackdrop(QImage &out);

    QWidget *m_backdropSource = nullptr;
    GlassPlate *m_plate = nullptr;
    QWidget *m_content = nullptr;

    bool m_needCapture = true;
    bool m_inBackdropGrab = false;
    bool m_backdropCaptureEnabled = true;

    qreal m_opacity = 0.65;
    QColor m_base{26, 26, 46};
    QColor m_border{255, 183, 197, 70};
    int m_radius = 16;
};
