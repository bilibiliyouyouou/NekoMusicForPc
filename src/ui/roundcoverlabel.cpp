#include "roundcoverlabel.h"

#include <QPainter>
#include <QPainterPath>

RoundCoverLabel::RoundCoverLabel(int radius, QWidget *parent)
    : QLabel(parent), m_radius(radius)
{
    setScaledContents(false);
    setAttribute(Qt::WA_TranslucentBackground);
}

void RoundCoverLabel::setPixmap(const QPixmap &pix)
{
    QLabel::setPixmap(pix);
    update();
}

void RoundCoverLabel::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    QPainterPath clip;
    clip.addRoundedRect(r, m_radius, m_radius);

    const QPixmap pm = pixmap();
    if (!pm.isNull()) {
        const QPixmap scaled = pm.scaled(r.size().toSize(), Qt::KeepAspectRatioByExpanding,
                                         Qt::SmoothTransformation);
        const qreal x = r.x() + (r.width() - scaled.width()) / 2.0;
        const qreal y = r.y() + (r.height() - scaled.height()) / 2.0;
        p.setClipPath(clip);
        p.drawPixmap(QPointF(x, y), scaled);
        return;
    }
    p.fillPath(clip, palette().window());
}
