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

/** 建议的 AUR 更新命令（如 yay -S neko-cloud-music），无助手时返回空 */
QString aurUpdateCommand();

/** 在终端中显示更新命令并进入交互 shell，由用户自行执行；无助手则打开 AUR 页面 */
bool launchAurUpdateInTerminal();
