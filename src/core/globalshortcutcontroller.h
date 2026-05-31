#pragma once

#include <QObject>

class QWindow;
class QWidget;
class AppShortcuts;

/** 全局播放快捷键：Wayland 走 xdg-desktop-portal GlobalShortcuts，X11 可回退本地抓取。 */
class GlobalShortcutController final : public QObject
{
    Q_OBJECT

public:
    enum class Backend {
        None,
        Portal,
        X11Grab,
        InAppFallback
    };
    Q_ENUM(Backend)

    static GlobalShortcutController &instance();

    void setHostWindow(QWindow *window);
    void installFallback(QWidget *parentWidget);
    void start();
    void rebind();
    void stop();
    void openSystemConfigureUi();

    Backend activeBackend() const { return m_backend; }
    bool isActive() const { return m_active; }
    QString statusText() const;

signals:
    void playPauseTriggered();
    void nextTrackTriggered();
    void previousTrackTriggered();
    void bindingStateChanged(bool active, Backend backend);
    void bindingFailed(const QString &reason);

private:
    explicit GlobalShortcutController(QObject *parent = nullptr);
    void dispatchAction(const QString &portalId);
    void activateBackend(Backend backend, bool active);
    void tryFallbackAfterPortalFailure(const QString &reason);

    QWindow *m_hostWindow = nullptr;
    Backend m_backend = Backend::None;
    bool m_active = false;

    struct BackendImpl;
    BackendImpl *m_impl = nullptr;
};
