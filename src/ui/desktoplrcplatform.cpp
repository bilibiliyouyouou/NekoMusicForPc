#include "desktoplrcplatform.h"

#include <QByteArray>
#include <QWindow>

#if defined(NEKO_HAS_KDE_WINDOWSYSTEM)
#include <KWindowSystem>
#include <KX11Extras>
#include <NETWM>
#endif

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#include <QGuiApplication>

#include <X11/Xatom.h>
#include <X11/Xlib.h>

namespace {

bool isX11Session()
{
    const QByteArray platform = qgetenv("QT_QPA_PLATFORM");
    if (platform.isEmpty())
        return qgetenv("WAYLAND_DISPLAY").isEmpty();
    return platform.contains("xcb") || platform.contains("x11");
}

void sendNetWmStateAbove(WId windowId)
{
    Display *display = XOpenDisplay(nullptr);
    if (!display || !windowId)
        return;

    const Atom netWmState = XInternAtom(display, "_NET_WM_STATE", False);
    const Atom aboveAtom = XInternAtom(display, "_NET_WM_STATE_ABOVE", False);

    XEvent event{};
    event.type = ClientMessage;
    event.xclient.window = windowId;
    event.xclient.message_type = netWmState;
    event.xclient.format = 32;
    event.xclient.data.l[0] = 1;
    event.xclient.data.l[1] = static_cast<long>(aboveAtom);
    event.xclient.data.l[2] = 0;
    XSendEvent(display, DefaultRootWindow(display), False,
               SubstructureRedirectMask | SubstructureNotifyMask, &event);
    XFlush(display);
    XCloseDisplay(display);
}

} // namespace
#endif

bool desktopLrcIsKdePlasmaSession()
{
    const QByteArray current = qgetenv("XDG_CURRENT_DESKTOP").toLower();
    if (current.contains("kde"))
        return true;
    const QByteArray session = qgetenv("XDG_SESSION_DESKTOP").toLower();
    return session.contains("kde") || session.contains("plasma");
}

bool desktopLrcIsWaylandSession()
{
#if defined(NEKO_HAS_KDE_WINDOWSYSTEM)
    if (KWindowSystem::platform() == KWindowSystem::Platform::Wayland)
        return true;
#endif
    const QByteArray platform = qgetenv("QT_QPA_PLATFORM");
    if (platform.contains("wayland"))
        return true;
    return !qgetenv("WAYLAND_DISPLAY").isEmpty();
}

void desktopLrcConfigureWindow(QWindow *window, bool useLayerShell)
{
    if (!window)
        return;

    if (useLayerShell) {
        // layer-shell 角色在首次 show 前由 LayerShellQt 绑定，此处不设 xdg_toplevel 标志
        return;
    }

    window->setFlags(window->flags() | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
}

void desktopLrcKeepOnTop(QWindow *window)
{
    if (!window || !window->isVisible())
        return;

    window->raise();

#if defined(NEKO_HAS_KDE_WINDOWSYSTEM)
    if (desktopLrcIsKdePlasmaSession() && KWindowSystem::platform() == KWindowSystem::Platform::X11) {
        KX11Extras::setState(window->winId(), NET::KeepAbove);
        return;
    }
#endif

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    if (isX11Session())
        sendNetWmStateAbove(window->winId());
#endif
}
