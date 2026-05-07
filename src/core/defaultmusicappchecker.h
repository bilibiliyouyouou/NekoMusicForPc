#pragma once

/**
 * @file defaultmusicappchecker.h
 * @brief 检测 / 尝试设置本应用为系统默认音乐打开方式（Linux / macOS / Windows）
 */

namespace DefaultMusicAppChecker {

/** 当前是否已被视为常见音频类型的默认打开程序 */
bool isDefaultMusicPlayer();

/**
 * 尝试设为默认（Linux: xdg-mime；macOS: LaunchServices；Windows: 打开「默认应用」设置页）
 */
void trySetAsDefaultMusicPlayer();

} // namespace DefaultMusicAppChecker
