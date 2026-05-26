#pragma once

/**
 * @file svgicon.h
 * @brief SPlayer SVG 图标渲染（resources/icons/*.svg）
 */

#include <QIcon>
#include <QPixmap>
#include <QString>
#include <QColor>

namespace Icons {

/** Qt 资源路径，name 不含 .svg，如 "Home" */
inline QString resourcePath(const char *name)
{
    return QStringLiteral(":/icons/%1.svg").arg(QLatin1String(name));
}

/** 按 SPlayer 图标名渲染 QPixmap */
QPixmap renderNamed(const char *name, int size, const QColor &color);

/** 按 SPlayer 图标名渲染 QIcon（Normal / Active 可选不同颜色） */
QIcon iconNamed(const char *name, int size, const QColor &normal,
                const QColor &active = QColor());

/** 从 Qt 资源路径加载 SVG（支持 fill="currentColor" 着色） */
QPixmap renderResource(const QString &resourcePath, int size, const QColor &color);
/** 按高度等比渲染宽标签类 SVG（如 HiRes） */
QPixmap renderResourceHeight(const QString &resourcePath, int height, const QColor &color);

/** @deprecated 请使用 renderNamed */
QPixmap render(const char *pathD, int size, const QColor &color, int viewBox = 24);

/** @deprecated 请使用 iconNamed */
QIcon icon(const char *pathD, int size, const QColor &normal,
           const QColor &active = QColor(), int viewBox = 24);

}  // namespace Icons
