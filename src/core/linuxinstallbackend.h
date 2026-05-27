#pragma once

#include <QString>

/** Linux 安装/更新渠道（用于应用内更新对话框） */
enum class LinuxInstallBackend {
    None,
    Debian, /**< dpkg 系：.deb */
    Arch,   /**< Arch / AUR：yay / paru / pacman */
};

LinuxInstallBackend detectLinuxInstallBackend();

/** AUR 包名（与 PKGBUILD pkgname 一致） */
QString aurPackageName();

/** AUR 包页面，无助手时打开浏览器 */
QString aurPackagePageUrl();

/** 已安装的 AUR 助手可执行名：yay、paru，无则返回空 */
QString findAurHelper();

/** 在终端中以当前用户执行 AUR 更新；无助手则打开 AUR 页面。返回是否启动了终端 */
bool launchAurUpdateInTerminal();
