#pragma once

class QWindow;

bool desktopLrcIsKdePlasmaSession();
bool desktopLrcIsWaylandSession();

/** 创建时设置窗口标志（layer-shell 路径在首次 show 前单独配置） */
void desktopLrcConfigureWindow(QWindow *window, bool useLayerShell);

/** 非 layer-shell 路径：显示期间提升到最前 */
void desktopLrcKeepOnTop(QWindow *window);
