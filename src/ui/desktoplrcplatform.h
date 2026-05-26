#pragma once

class QWidget;

bool desktopLrcIsKdePlasmaSession();
bool desktopLrcIsWaylandSession();

/** 创建时设置：置顶、无边框悬浮工具窗、无最小化/最大化（layer-shell 路径不在此改 flags） */
void desktopLrcConfigureWindow(QWidget *window, bool useLayerShell);

/** 非 layer-shell 路径：显示期间提升到最前 */
void desktopLrcKeepOnTop(QWidget *window);
