#pragma once

#include <QRect>

class QPainter;
class QColor;
class GlassWidget;

/** 主窗口背景与条带面板（纯 QPainter，无 OpenGL）。 */
namespace GlassPaint {

enum class BarKind {
    TitleBar,
    Sidebar,
    PlayerBar,
};

void paintMainWindowDeepBackdrop(QPainter &p, const QRect &r, bool darkMode);

void paintBarGlass(QPainter &p, const QRect &r, BarKind kind, bool darkMode);

/** SPlayer 式扁平卡片/面板：禁用 backdrop 抓取，实心 surface */
void applyFlatSurface(GlassWidget *glass, bool darkMode);

} // namespace GlassPaint
