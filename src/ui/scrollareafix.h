#pragma once

#include <QScrollArea>
#include <QWidget>

/** Windows Fusion 下 QScrollArea 视口常带不透明系统背景，会盖住父级毛玻璃/渐变。 */
inline void nekoPolishScrollAreaViewport(QScrollArea *scroll)
{
    if (!scroll)
        return;
    scroll->setAutoFillBackground(false);
    if (QWidget *vp = scroll->viewport()) {
        vp->setAutoFillBackground(false);
        vp->setAttribute(Qt::WA_NoSystemBackground, true);
    }
}
