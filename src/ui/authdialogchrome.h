#pragma once

#include "theme/theme.h"
#include "theme/thememanager.h"

#include <QColor>
#include <QString>

namespace AuthDialogChrome {

constexpr int kDialogWidth = 440;
constexpr int kOuterPad = 24;
constexpr int kCardPadH = 28;
constexpr int kCardPadV = 26;
constexpr int kSectionSpacing = 20;
constexpr int kFieldSpacing = 14;
constexpr int kFieldHeight = 44;
constexpr int kPrimaryBtnHeight = 46;
constexpr int kLinkBtnHeight = 38;

struct Palette {
    QString cardBg;
    QString cardBorder;
    QString titleColor;
    QString bodyColor;
    QString msgColor;
};

inline Palette palette(bool dark)
{
    if (dark) {
        return {
            QStringLiteral("rgba(36, 31, 49, 245)"),
            QStringLiteral("rgba(230, 57, 80, 60)"),
            QString::fromUtf8(Theme::kLavender),
            QString::fromUtf8(Theme::kTextSub),
            QString::fromUtf8(Theme::kSakura),
        };
    }
    return {
        QStringLiteral("rgba(255, 255, 255, 0.98)"),
        QStringLiteral("rgba(111, 66, 193, 0.28)"),
        QStringLiteral("#6F42C1"),
        QStringLiteral("rgba(33, 37, 41, 0.72)"),
        QStringLiteral("#D62839"),
    };
}

inline Palette currentPalette()
{
    return palette(Theme::ThemeManager::instance().isDarkMode());
}

inline QString cardStyleSheet(const Palette &p)
{
    return QStringLiteral(
               "QWidget#authDialogCard {"
               "  background: %1;"
               "  border: 1px solid %2;"
               "  border-radius: 16px;"
               "}")
        .arg(p.cardBg, p.cardBorder);
}

inline QString titleStyleSheet(const Palette &p)
{
    return QStringLiteral(
               "QLabel { color: %1; font-size: 22px; font-weight: bold; padding: 2px 0 10px 0; }")
        .arg(p.titleColor);
}

inline QString bodyStyleSheet(const Palette &p)
{
    return QStringLiteral("QLabel { color: %1; font-size: 13px; }").arg(p.bodyColor);
}

inline QString msgStyleSheet(const QString &color)
{
    return QStringLiteral("QLabel { color: %1; font-size: 13px; min-height: 20px; }").arg(color);
}

} // namespace AuthDialogChrome
