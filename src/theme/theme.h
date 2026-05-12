#pragma once

/**
 * @file theme.h
 * @brief NekoMusic 桌面端主题常量（与 Android 端玫红主色 + 深夜蓝底对齐，现代 ACG）
 *
 * 玫红强调 + 樱花粉描边 + 薄荷/天蓝焦点环；无 emoji。尺寸与动画集中于此。
 */

#include <QColor>

namespace Theme
{

    // ─── 主色：玫红（与 Android Color.kt RoseRed / LightRose 一致）────────
    constexpr const char *kLavender = "#FF6B8B";   // 主强调（沿用旧名以免全工程改名）
    constexpr const char *kLavenderLt = "#FF8FA3";
    constexpr const char *kLavenderDk = "#E63950";

    // ─── 辅色：樱花粉（玻璃描边 / 弱高亮）────────────────────────────
    constexpr const char *kSakura = "#FFB7C5";
    constexpr const char *kSakuraLt = "#FFD0DA";
    constexpr const char *kSakuraDk = "#F2ACB9";

    // ─── 点缀：薄荷绿（焦点 / 侧栏选中条）────────────────────────────
    constexpr const char *kMint = "#7EC8C8";
    constexpr const char *kMintLt = "#9DD8D8";
    constexpr const char *kMintDk = "#5EAEAE";

    // ─── 背景（深夜蓝紫，贴近 Android BackgroundDark / Surface）────────
    constexpr const char *kBgDeep = "#0E0E1C";
    constexpr const char *kBgMid = "#121228";
    constexpr const char *kBgSurface = "#1A1A2E";

    // ─── 文字 ────────────────────────────────────────────────────────
    constexpr const char *kTextMain = "#F4F6FF";
    constexpr const char *kTextSub = "rgba(244, 246, 255, 168)";
    constexpr const char *kTextMuted = "rgba(244, 246, 255, 105)";

    // ─── 毛玻璃（基于新 surface）────────────────────────────────────
    constexpr const char *kGlassBg = "rgba(26, 26, 46, 178)";
    constexpr const char *kGlassSidebar = "rgba(18, 18, 40, 210)";
    constexpr const char *kGlassPlayer = "rgba(18, 18, 40, 225)";
    constexpr const char *kGlassOverlay = "rgba(14, 14, 28, 185)";

    // ─── 边框（樱花粉半透明，呼应 Android GlassSurface 描边）──────────
    constexpr const char *kBorderGlass = "rgba(255, 183, 197, 55)";
    constexpr const char *kBorderFocus = "rgba(135, 206, 235, 140)";

    // ─── 渐变（QSS）──────────────────────────────────────────────────
    constexpr const char *kGradMain = "qlineargradient(x1:0,y1:0,x2:0.28,y2:1,"
                                      "stop:0 #FF8FA3, stop:1 #E63950)";
    constexpr const char *kGradSakura = "qlineargradient(x1:0,y1:0,x2:0.3,y2:1,"
                                          "stop:0 #FFD0DA, stop:1 #F2ACB9)";
    constexpr const char *kGradMint = "qlineargradient(x1:0,y1:0,x2:0.3,y2:1,"
                                        "stop:0 #7EC8C8, stop:1 #5EAEAE)";
    constexpr const char *kGradBg = "qlineargradient(x1:0,y1:0,x2:0.45,y2:1,"
                                    "stop:0 #080810, stop:0.35 #0E0E1C, stop:1 #15152A)";

    // ─── 布局尺寸 ────────────────────────────────────────────────────
    constexpr int kSidebarW = 248;
    constexpr int kTitleBarH = 58;
    constexpr int kPlayerBarH = 84;
    constexpr int kCoverSmall = 144;
    constexpr int kCoverRadius = 14;

    // ─── 圆角 ──────────────────────────────────────────────────────
    constexpr int kRSm = 10;
    constexpr int kRMd = 14;
    constexpr int kRLg = 18;
    constexpr int kRXl = 22;

    // ─── 动画 (ms) ─────────────────────────────────────────────────
    constexpr int kAnimFast = 150;
    constexpr int kAnimNormal = 250;
    constexpr int kAnimSlow = 400;
    constexpr int kCarouselMs = 5000;

    constexpr const char *kApiBase = "https://music.cnmsb.xin";

} // namespace Theme
