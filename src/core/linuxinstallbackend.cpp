#include "linuxinstallbackend.h"

#include <QDesktopServices>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>

LinuxInstallBackend detectLinuxInstallBackend()
{
#if !defined(Q_OS_LINUX)
    return LinuxInstallBackend::None;
#else
    if (QFile::exists(QStringLiteral("/etc/arch-release")))
        return LinuxInstallBackend::Arch;
    if (QFile::exists(QStringLiteral("/etc/debian_version"))
        || QFile::exists(QStringLiteral("/etc/debian_release")))
        return LinuxInstallBackend::Debian;
    if (!QStandardPaths::findExecutable(QStringLiteral("dpkg")).isEmpty())
        return LinuxInstallBackend::Debian;
    return LinuxInstallBackend::None;
#endif
}

QString aurPackageName()
{
    return QStringLiteral("neko-cloud-music");
}

QString aurPackagePageUrl()
{
    return QStringLiteral("https://aur.archlinux.org/packages/neko-cloud-music");
}

QString findAurHelper()
{
#if !defined(Q_OS_LINUX)
    return {};
#else
    if (!QStandardPaths::findExecutable(QStringLiteral("yay")).isEmpty())
        return QStringLiteral("yay");
    if (!QStandardPaths::findExecutable(QStringLiteral("paru")).isEmpty())
        return QStringLiteral("paru");
    return {};
#endif
}

bool launchAurUpdateInTerminal()
{
#if !defined(Q_OS_LINUX)
    return false;
#else
    const QString helper = findAurHelper();
    const QString pkg = aurPackageName();

    if (helper.isEmpty()) {
        QDesktopServices::openUrl(QUrl(aurPackagePageUrl()));
        return false;
    }

    const QString inner = QStringLiteral("%1 -S --needed %2; echo; read -p \"Press Enter to close...\" _")
                              .arg(helper, pkg);

    const auto tryLaunch = [&](const QString &program, const QStringList &args) -> bool {
        if (QStandardPaths::findExecutable(program).isEmpty())
            return false;
        return QProcess::startDetached(program, args);
    };

    if (tryLaunch(QStringLiteral("konsole"),
                  {QStringLiteral("-e"), QStringLiteral("bash"), QStringLiteral("-c"), inner}))
        return true;
    if (tryLaunch(QStringLiteral("gnome-terminal"),
                  {QStringLiteral("--"), QStringLiteral("bash"), QStringLiteral("-c"), inner}))
        return true;
    if (tryLaunch(QStringLiteral("xfce4-terminal"),
                  {QStringLiteral("-e"), QStringLiteral("bash"), QStringLiteral("-c"), inner}))
        return true;
    if (tryLaunch(QStringLiteral("alacritty"),
                  {QStringLiteral("-e"), QStringLiteral("bash"), QStringLiteral("-c"), inner}))
        return true;
    if (tryLaunch(QStringLiteral("kitty"),
                  {QStringLiteral("bash"), QStringLiteral("-c"), inner}))
        return true;
    if (tryLaunch(QStringLiteral("foot"),
                  {QStringLiteral("bash"), QStringLiteral("-c"), inner}))
        return true;
    if (tryLaunch(QStringLiteral("xterm"),
                  {QStringLiteral("-e"), QStringLiteral("bash"), QStringLiteral("-c"), inner}))
        return true;

    QDesktopServices::openUrl(QUrl(aurPackagePageUrl()));
    return false;
#endif
}
