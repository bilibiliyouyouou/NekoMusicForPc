/**
 * @file glasswidget.cpp
 * @brief 圆角液态玻璃：隐藏自身后抓取父控件区域，经轻量缩放柔化后叠底色与描边（QPainter，全平台一致）。
 */

#include "glasswidget.h"

#include "theme/thememanager.h"

#include <QImage>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QResizeEvent>
#include <QStackedLayout>
#include <QWidget>

class GlassPlate final : public QWidget
{
public:
    explicit GlassPlate(GlassWidget *owner) : m_owner(owner)
    {
        setAttribute(Qt::WA_TranslucentBackground, true);
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAutoFillBackground(false);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

protected:
    void resizeEvent(QResizeEvent *e) override
    {
        if (m_owner)
            m_owner->m_needCapture = true;
        QWidget::resizeEvent(e);
    }

    void paintEvent(QPaintEvent *) override
    {
        if (!m_owner || m_owner->width() < 2 || m_owner->height() < 2)
            return;

        ensureGrab();

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        const QRect r = rect();
        const qreal rad = qreal(m_owner->m_radius);
        QPainterPath clip;
        clip.addRoundedRect(QRectF(r), rad, rad);
        p.setClipPath(clip);

        const bool dark = Theme::ThemeManager::instance().isDarkMode();
        const qreal op = qBound(0.0, m_owner->m_opacity, 1.0);
        const bool flat = !m_owner->m_backdropCaptureEnabled;

        if (!flat && !m_grab.isNull())
            p.drawPixmap(r, m_grab);
        else {
            QColor fill = m_owner->m_base;
            if (flat || fill.alpha() < 250)
                fill.setAlpha(255);
            p.fillRect(r, fill);
        }

        if (!flat) {
            QColor tint = m_owner->m_base;
            tint.setAlphaF(op * (dark ? 0.42 : 0.38));
            p.fillRect(r, tint);

            QLinearGradient hi(r.topLeft(), QPoint(r.left(), r.top() + qMin(r.height() / 3, 120)));
            hi.setColorAt(0.0, dark ? QColor(255, 255, 255, int(28 * op + 8))
                                    : QColor(255, 255, 255, int(55 * op + 15)));
            hi.setColorAt(1.0, Qt::transparent);
            p.fillRect(r, hi);
        }

        p.setClipping(false);
        const qreal dpr = qMax(1.0, m_owner->devicePixelRatioF());
        QPen pen(m_owner->m_border);
        pen.setWidthF(qMax(1.0, dpr * 0.55));
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        const qreal inset = pen.widthF() * 0.5;
        p.drawRoundedRect(QRectF(r).adjusted(inset, inset, -inset, -inset), rad - inset, rad - inset);
    }

private:
    void ensureGrab()
    {
        if (!m_owner->parentWidget())
            return;

        if (!m_owner->m_backdropCaptureEnabled) {
            m_grab = QPixmap();
            m_cachedLogical = m_owner->size();
            m_cachedDpr = qMax(1.0, m_owner->devicePixelRatioF());
            m_owner->m_needCapture = false;
            return;
        }

        const QSize logical = m_owner->size();
        const qreal dpr = qMax(1.0, m_owner->devicePixelRatioF());
        const bool need = m_owner->m_needCapture || logical != m_cachedLogical || !qFuzzyCompare(dpr, m_cachedDpr)
                          || m_grab.isNull();

        if (!need)
            return;

        m_owner->m_needCapture = false;

        QImage cap;
        m_owner->captureBackdrop(cap);

        if (!cap.isNull()) {
            int w = cap.width();
            int h = cap.height();
            if (w > 12 && h > 12) {
                QImage soft = cap.scaled(w / 2, h / 2, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                                  .scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                m_grab = QPixmap::fromImage(soft);
            } else {
                m_grab = QPixmap::fromImage(cap);
            }
        } else {
            m_grab = QPixmap();
        }

        m_cachedLogical = logical;
        m_cachedDpr = dpr;
    }

    GlassWidget *m_owner = nullptr;
    QPixmap m_grab;
    QSize m_cachedLogical;
    qreal m_cachedDpr = 0;
};

GlassWidget::GlassWidget(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAutoFillBackground(false);

    m_plate = new GlassPlate(this);
    m_content = new QWidget(this);
    m_content->setAttribute(Qt::WA_TranslucentBackground, true);
    m_content->setAttribute(Qt::WA_NoSystemBackground, true);
    m_content->setAutoFillBackground(false);
    m_content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto *stack = new QStackedLayout(this);
    stack->setStackingMode(QStackedLayout::StackAll);
    stack->setContentsMargins(0, 0, 0, 0);
    stack->addWidget(m_plate);
    stack->addWidget(m_content);
    // StackAll 下后加的控件会 lower() 到最底，当前页仍是 m_plate，会把整层内容挡住。
    // 把当前页切到 content，setCurrentIndex 会对该页 raise()，玻璃层留在下面。
    stack->setCurrentWidget(m_content);

    connect(&Theme::ThemeManager::instance(), &Theme::ThemeManager::themeChanged, this,
            [this](Theme::ThemeMode) { refreshBackdrop(); });
}

GlassWidget::~GlassWidget() = default;

qreal GlassWidget::opacity() const { return m_opacity; }
void GlassWidget::setOpacity(qreal v)
{
    m_opacity = v;
    if (m_plate)
        m_plate->update();
    update();
}

QColor GlassWidget::baseColor() const { return m_base; }
void GlassWidget::setBaseColor(const QColor &c)
{
    m_base = c;
    if (m_plate)
        m_plate->update();
    update();
}

QColor GlassWidget::borderColor() const { return m_border; }
void GlassWidget::setBorderColor(const QColor &c)
{
    m_border = c;
    if (m_plate)
        m_plate->update();
    update();
}

int GlassWidget::borderRadius() const { return m_radius; }
void GlassWidget::setBorderRadius(int r)
{
    m_radius = r;
    m_needCapture = true;
    if (m_plate)
        m_plate->update();
    update();
}

void GlassWidget::setBackdropCaptureEnabled(bool enabled)
{
    if (m_backdropCaptureEnabled == enabled)
        return;
    m_backdropCaptureEnabled = enabled;
    refreshBackdrop();
}

void GlassWidget::refreshBackdrop()
{
    m_needCapture = true;
    if (m_plate)
        m_plate->update();
    update();
}

void GlassWidget::captureBackdrop(QImage &out)
{
    out = QImage();
    QWidget *pw = m_backdropSource ? m_backdropSource : parentWidget();
    if (!pw || width() < 2 || height() < 2)
        return;

    const QPoint tl = mapTo(pw, QPoint(0, 0));
    const QRect grabRect(tl, size());

    const bool vis = isVisible();
    setVisible(false);
    QPixmap pm = pw->grab(grabRect);
    setVisible(vis);

    if (pm.isNull())
        return;

    out = pm.toImage().convertToFormat(QImage::Format_RGBA8888);
}
