#include "desktoplrcplatform.h"

#include <QByteArray>
#include <QWidget>
#include <QWindow>

#if defined(Q_OS_WIN)
#include <Windows.h>
#endif

#if defined(NEKO_HAS_KDE_WINDOWSYSTEM)
#include <KWindowSystem>
#include <KX11Extras>
#include <NETWM>
#endif

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#include <QGuiApplication>
#include <QtGui/qguiapplication_platform.h>

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

void sendNetWmStateAboveOnDisplay(Display *display, WId windowId)
{
    if (!display || !windowId)
        return;

    const ::Window xw = static_cast<::Window>(windowId);
    const Atom netWmState = XInternAtom(display, "_NET_WM_STATE", False);
    const Atom aboveAtom = XInternAtom(display, "_NET_WM_STATE_ABOVE", False);

    XEvent event{};
    event.type = ClientMessage;
    event.xclient.window = xw;
    event.xclient.message_type = netWmState;
    event.xclient.format = 32;
    event.xclient.data.l[0] = 1; // add
    event.xclient.data.l[1] = static_cast<long>(aboveAtom);
    event.xclient.data.l[2] = 0;
    // EWMH: source + timestamp improve stacking on mutter/kwin etc.
    event.xclient.data.l[3] = 1; // normal application
    event.xclient.data.l[4] = static_cast<long>(CurrentTime);
    XSendEvent(display, DefaultRootWindow(display), False,
               SubstructureRedirectMask | SubstructureNotifyMask, &event);
    XFlush(display);
}

void sendNetWmStateAbove(WId windowId)
{
    if (QGuiApplication *app = qGuiApp) {
        if (auto *x11 = app->nativeInterface<QNativeInterface::QX11Application>()) {
            if (Display *dpy = x11->display()) {
                sendNetWmStateAboveOnDisplay(dpy, windowId);
                return;
            }
        }
    }
    if (Display *dpy = XOpenDisplay(nullptr)) {
        sendNetWmStateAboveOnDisplay(dpy, windowId);
        XCloseDisplay(dpy);
    }
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

void desktopLrcConfigureWindow(QWidget *window, bool useLayerShell)
{
    if (!window)
        return;

    if (useLayerShell) {
        // layer-shell 在首次 show 前由 LayerShellQt 绑定；勿在此处覆盖 flags，避免与 overlay 初始化冲突
        return;
    }

    // 置顶 + 无边框工具窗（悬浮、常不占任务栏）+ 去掉最小化/最大化按钮位
    Qt::WindowFlags flags = Qt::Window | Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint;
    flags &= ~(Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint
               | Qt::WindowContextHelpButtonHint);
    window->setWindowFlags(flags);
}

void desktopLrcKeepOnTop(QWidget *window)
{
    if (!window || !window->isVisible())
        return;

    if (QWindow *handle = window->windowHandle())
        handle->raise();

#if defined(Q_OS_WIN)
    // Qt::WindowStaysOnTopHint 可被部分程序压在下面；HWND_TOPMOST 与系统「置顶」一致
    if (HWND hwnd = reinterpret_cast<HWND>(window->winId())) {
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }
#endif

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
