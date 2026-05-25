#include "glasspaint.h"
#include "glasswidget.h"

#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QtMath>

#include <cmath>

namespace {

QPixmap noiseTile()
{
    static QPixmap cache;
    if (!cache.isNull())
        return cache;

    constexpr int n = 96;
    QImage img(n, n, QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            const qreal t = qSin(qreal(x) * 12.9898 + qreal(y) * 78.233) * 43758.5453;
            int v = int(t) & 255;
            v = (v - 128) / 8;
            const int g = qBound(0, 128 + v, 255);
            img.setPixelColor(x, y, QColor(g, g, g, 18));
        }
    }
    cache = QPixmap::fromImage(img);
    return cache;
}

void drawNoiseOverlay(QPainter &p, const QRect &r, qreal strength = 1.0)
{
    const QPixmap tile = noiseTile();
    if (tile.isNull())
        return;
    p.save();
    p.setOpacity(0.028 * strength);
    p.drawTiledPixmap(r, tile);
    p.restore();
}

float circleMap(float x)
{
    x = qBound(0.f, x, 1.f);
    return 1.f - std::sqrt(std::max(0.f, 1.f - x * x));
}

} // namespace

namespace GlassPaint {

void paintMainWindowDeepBackdrop(QPainter &p, const QRect &r, bool darkMode)
{
    p.setRenderHint(QPainter::Antialiasing);

    QLinearGradient bg(r.topLeft(), r.bottomLeft());
    if (darkMode) {
        bg.setColorAt(0.0, QColor(24, 24, 24));
        bg.setColorAt(1.0, QColor(20, 20, 20));
    } else {
        bg.setColorAt(0.0, QColor(251, 251, 252));
        bg.setColorAt(1.0, QColor(241, 243, 246));
    }
    p.fillRect(r, bg);
}

void paintBarGlass(QPainter &p, const QRect &r, BarKind kind, bool darkMode)
{
    const QColor surface = darkMode ? QColor(30, 30, 30) : QColor(255, 255, 255);
    const QColor border = darkMode ? QColor(255, 255, 255, 18) : QColor(0, 0, 0, 22);
    p.fillRect(r, surface);

    p.setPen(QPen(border, 1));
    if (kind == BarKind::Sidebar)
        p.drawLine(r.topRight(), r.bottomRight());
    else if (kind == BarKind::PlayerBar)
        p.drawLine(r.topLeft(), r.topRight());
    else
        p.drawLine(r.bottomLeft(), r.bottomRight());
}

void applyFlatSurface(GlassWidget *glass, bool darkMode)
{
    if (!glass)
        return;
    glass->setBackdropCaptureEnabled(false);
    if (darkMode) {
        glass->setBaseColor(QColor(36, 36, 36));
        glass->setBorderColor(QColor(255, 255, 255, 18));
    } else {
        glass->setBaseColor(QColor(255, 255, 255));
        glass->setBorderColor(QColor(0, 0, 0, 22));
    }
    glass->setOpacity(1.0);
}

} // namespace GlassPaint
