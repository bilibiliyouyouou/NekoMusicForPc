#pragma once

class QWidget;

bool desktopLrcIsKdePlasmaSession();
bool desktopLrcIsWaylandSession();

/** 创建时设置窗口标志（layer-shell 路径在首次 show 前单独配置） */
void desktopLrcConfigureWindow(QWidget *window, bool useLayerShell);

/** 非 layer-shell 路径：显示期间提升到最前 */
void desktopLrcKeepOnTop(QWidget *window);
