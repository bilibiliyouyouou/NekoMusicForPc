#include "glasspaint.h"

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

    /* 与 style.qss / style-light.qss 中 QMainWindow 渐变一致 */
    QLinearGradient bg(r.topLeft(), QPoint(int(r.width() * 0.42), r.height()));
    if (darkMode) {
        bg.setColorAt(0.0, QColor(20, 16, 28));    /* #14101c */
        bg.setColorAt(0.38, QColor(26, 22, 38));  /* #1a1626 */
        bg.setColorAt(1.0, QColor(37, 32, 50));  /* #252032 */
    } else {
        bg.setColorAt(0.0, QColor(251, 251, 252));
        bg.setColorAt(0.45, QColor(241, 243, 246));
        bg.setColorAt(1.0, QColor(232, 236, 241));
    }
    p.fillRect(r, bg);

    QRadialGradient topGlow(QPointF(r.center().x(), r.top() + r.height() * 0.08), r.width() * 0.75);
    if (darkMode) {
        topGlow.setColorAt(0.0, QColor(196, 167, 231, 34));
        topGlow.setColorAt(0.42, QColor(126, 200, 200, 14));
        topGlow.setColorAt(1.0, Qt::transparent);
    } else {
        topGlow.setColorAt(0.0, QColor(196, 167, 231, 48));
        topGlow.setColorAt(0.48, QColor(255, 255, 255, 88));
        topGlow.setColorAt(1.0, Qt::transparent);
    }
    p.fillRect(r, topGlow);

    const int vx = int(r.width() * 0.35);
    const int vy = int(r.height() * 0.4);
    QRadialGradient vLeft(r.bottomLeft(), qMax(vx, vy));
    vLeft.setColorAt(0.0, darkMode ? QColor(8, 6, 14, 100) : QColor(108, 117, 125, 26));
    vLeft.setColorAt(1.0, Qt::transparent);
    p.fillRect(r, vLeft);

    QRadialGradient vRight(r.bottomRight(), qMax(vx, vy));
    vRight.setColorAt(0.0, darkMode ? QColor(8, 6, 14, 92) : QColor(108, 117, 125, 20));
    vRight.setColorAt(1.0, Qt::transparent);
    p.fillRect(r, vRight);

    drawNoiseOverlay(p, r, darkMode ? 0.92 : 0.78);
}

void paintBarGlass(QPainter &p, const QRect &r, BarKind kind, bool darkMode)
{
    p.setRenderHint(QPainter::Antialiasing);

    QLinearGradient depth(r.topLeft(), r.bottomLeft());
    if (darkMode) {
        depth.setColorAt(0.0, QColor(40, 34, 56, 238));
        depth.setColorAt(0.52, QColor(30, 26, 42, 230));
        depth.setColorAt(1.0, QColor(22, 18, 32, 222));
    } else {
        depth.setColorAt(0.0, QColor(255, 255, 255, 250));
        depth.setColorAt(0.5, QColor(248, 249, 250, 244));
        depth.setColorAt(1.0, QColor(236, 239, 242, 238));
    }
    p.fillRect(r, depth);

    QRadialGradient hi(QPointF(r.left() + r.width() * 0.08, r.top() + r.height() * 0.12), r.height() * 1.1);
    if (darkMode) {
        hi.setColorAt(0.0, QColor(255, 255, 255, 26));
        hi.setColorAt(0.35, QColor(196, 167, 231, 22));
        hi.setColorAt(1.0, Qt::transparent);
    } else {
        hi.setColorAt(0.0, QColor(255, 255, 255, 200));
        hi.setColorAt(0.4, QColor(196, 167, 231, 45));
        hi.setColorAt(1.0, Qt::transparent);
    }
    p.fillRect(r, hi);

    drawNoiseOverlay(p, r, 0.9);

    {
        const int stripH = qBound(3, r.height() / 22, 18);
        constexpr float kPi = 3.14159265f;
        p.save();
        for (int py = 0; py < stripH; ++py) {
            const float t = qBound(0.f, 1.f - (py + 0.35f) / float(stripH), 1.f);
            float lens = circleMap(t);
            lens *= 0.88f + 0.12f * std::sin(kPi * float(py + 1) / float(stripH + 1));

            const int a = qBound(0, int(std::lround(255.f * lens * (darkMode ? 0.21f : 0.36f))), 255);
            if (a < 2)
                continue;

            const float hueShift = std::sin(float(py) * 0.85f) * 0.5f + 0.5f;
            int r0, g0, b0;
            if (darkMode) {
                r0 = qBound(0, int(175 + 40 * hueShift), 255);
                g0 = qBound(0, int(205 + 35 * (1.f - hueShift * 0.5f)), 255);
                b0 = qBound(0, int(248 - 15 * hueShift), 255);
            } else {
                r0 = qBound(0, int(228 + 25 * hueShift), 255);
                g0 = qBound(0, int(238 + 20 * (1.f - hueShift * 0.5f)), 255);
                b0 = 255;
            }
            p.fillRect(r.left(), r.top() + py, r.width(), 1, QColor(r0, g0, b0, a));
        }
        p.restore();
    }

    QLinearGradient accent;
    switch (kind) {
    case BarKind::TitleBar:
        accent = QLinearGradient(r.topLeft(), r.topRight());
        accent.setColorAt(0.0, QColor(196, 167, 231, 0));
        accent.setColorAt(0.5, QColor(196, 167, 231, darkMode ? 42 : 52));
        accent.setColorAt(1.0, QColor(196, 167, 231, 0));
        p.setPen(QPen(QBrush(accent), 1));
        p.drawLine(r.bottomLeft(), r.bottomRight());
        break;
    case BarKind::Sidebar:
        accent = QLinearGradient(r.topRight(), r.bottomRight());
        accent.setColorAt(0.0, QColor(196, 167, 231, darkMode ? 38 : 50));
        accent.setColorAt(1.0, QColor(196, 167, 231, darkMode ? 12 : 18));
        p.setPen(QPen(QBrush(accent), 1));
        p.drawLine(r.topRight(), r.bottomRight());
        break;
    case BarKind::PlayerBar:
        accent = QLinearGradient(r.topLeft(), r.topRight());
        accent.setColorAt(0.0, QColor(196, 167, 231, 0));
        accent.setColorAt(0.5, QColor(196, 167, 231, darkMode ? 46 : 54));
        accent.setColorAt(1.0, QColor(196, 167, 231, 0));
        p.setPen(QPen(QBrush(accent), 1));
        p.drawLine(r.topLeft(), r.topRight());
        break;
    }
}

} // namespace GlassPaint
