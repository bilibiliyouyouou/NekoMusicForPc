#pragma once

#include <QRect>

class QPainter;
class QColor;

/**
 * KDE Plasma 风格的「软件毛玻璃」绘制：主窗口背景与条带面板（QPainter）。
 * 圆角卡片液态效果由 GlassWidget（QPainter + backdrop 抓取）实现。
 */
namespace GlassPaint {

enum class BarKind {
    TitleBar,
    Sidebar,
    PlayerBar,
};

void paintMainWindowDeepBackdrop(QPainter &p, const QRect &r, bool darkMode);

void paintBarGlass(QPainter &p, const QRect &r, BarKind kind, bool darkMode);

} // namespace GlassPaint
