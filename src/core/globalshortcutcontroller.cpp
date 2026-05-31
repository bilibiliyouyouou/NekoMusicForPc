#include "globalshortcutcontroller.h"
#include "appshortcuts.h"

#include "core/i18n.h"

#include <QShortcut>
#include <QTimer>
#include <QWidget>
#include <QWindow>
#include <functional>

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
#include "globalshortcutportal_linux.h"
#endif

namespace {

class InAppFallbackBackend final : public QObject
{
public:
    explicit InAppFallbackBackend(QWidget *parentWidget)
        : QObject(parentWidget), m_parent(parentWidget)
    {
    }

    void reload(const std::function<void(AppShortcuts::Action)> &onAction)
    {
        qDeleteAll(m_shortcuts);
        m_shortcuts.clear();

        auto bind = [this, &onAction](const QKeySequence &seq, AppShortcuts::Action action) {
            if (seq.isEmpty())
                return;
            auto *sc = new QShortcut(seq, m_parent);
            sc->setContext(Qt::ApplicationShortcut);
            connect(sc, &QShortcut::activated, this, [onAction, action]() { onAction(action); });
            m_shortcuts.append(sc);
        };

        const AppShortcuts &app = AppShortcuts::instance();
        bind(app.sequence(AppShortcuts::PlayPause), AppShortcuts::PlayPause);
        bind(app.sequence(AppShortcuts::NextTrack), AppShortcuts::NextTrack);
        bind(app.sequence(AppShortcuts::PreviousTrack), AppShortcuts::PreviousTrack);
    }

    void stop()
    {
        qDeleteAll(m_shortcuts);
        m_shortcuts.clear();
    }

    bool hasShortcuts() const { return !m_shortcuts.isEmpty(); }

private:
    QWidget *m_parent;
    QList<QShortcut *> m_shortcuts;
};

} // namespace

struct GlobalShortcutController::BackendImpl
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    GlobalShortcutPortalLinux *portal = nullptr;
#endif
    InAppFallbackBackend *fallback = nullptr;
};

GlobalShortcutController &GlobalShortcutController::instance()
{
    static GlobalShortcutController inst;
    return inst;
}

GlobalShortcutController::GlobalShortcutController(QObject *parent)
    : QObject(parent), m_impl(new BackendImpl)
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    m_impl->portal = new GlobalShortcutPortalLinux(this);
    connect(m_impl->portal, &GlobalShortcutPortalLinux::shortcutActivated, this,
            &GlobalShortcutController::dispatchAction);
    connect(m_impl->portal, &GlobalShortcutPortalLinux::bindSucceeded, this, [this]() {
        activateBackend(Backend::Portal, true);
    });
    connect(m_impl->portal, &GlobalShortcutPortalLinux::bindFailed, this,
            &GlobalShortcutController::tryFallbackAfterPortalFailure);
#endif
}

void GlobalShortcutController::installFallback(QWidget *parentWidget)
{
    if (!parentWidget || m_impl->fallback)
        return;
    m_impl->fallback = new InAppFallbackBackend(parentWidget);
}

void GlobalShortcutController::setHostWindow(QWindow *window)
{
    m_hostWindow = window;
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    if (m_impl->portal)
        m_impl->portal->setHostWindow(window);
#endif
}

void GlobalShortcutController::activateBackend(Backend backend, bool active)
{
    m_backend = backend;
    m_active = active;
    emit bindingStateChanged(active, backend);
}

void GlobalShortcutController::tryFallbackAfterPortalFailure(const QString &reason)
{
    if (m_impl->fallback) {
        m_impl->fallback->reload([this](AppShortcuts::Action action) {
            switch (action) {
            case AppShortcuts::PlayPause:
                emit playPauseTriggered();
                break;
            case AppShortcuts::NextTrack:
                emit nextTrackTriggered();
                break;
            case AppShortcuts::PreviousTrack:
                emit previousTrackTriggered();
                break;
            default:
                break;
            }
        });
        if (m_impl->fallback->hasShortcuts()) {
            activateBackend(Backend::InAppFallback, true);
            emit bindingFailed(I18n::instance().tr(QStringLiteral("shortcutGlobalPortalFailed"))
                                   .arg(reason));
            return;
        }
    }

    m_active = false;
    m_backend = Backend::None;
    emit bindingFailed(reason);
}

void GlobalShortcutController::start()
{
    stop();

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    if (m_impl->portal && GlobalShortcutPortalLinux::isAvailable()) {
        m_impl->portal->bind();
        return;
    }
#endif

    tryFallbackAfterPortalFailure(I18n::instance().tr(QStringLiteral("shortcutGlobalUnavailable")));
}

void GlobalShortcutController::rebind()
{
    stop();
    QTimer::singleShot(0, this, &GlobalShortcutController::start);
}

void GlobalShortcutController::stop()
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    if (m_impl->portal)
        m_impl->portal->unbind();
#endif
    if (m_impl->fallback)
        m_impl->fallback->stop();

    m_active = false;
    m_backend = Backend::None;
}

void GlobalShortcutController::openSystemConfigureUi()
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    if (m_impl->portal)
        m_impl->portal->openConfigureUi();
#endif
}

QString GlobalShortcutController::statusText() const
{
    switch (m_backend) {
    case Backend::Portal:
        return I18n::instance().tr(QStringLiteral("shortcutGlobalPortalActive"));
    case Backend::InAppFallback:
        return I18n::instance().tr(QStringLiteral("shortcutGlobalFallbackActive"));
    default:
        return I18n::instance().tr(QStringLiteral("shortcutGlobalInactive"));
    }
}

void GlobalShortcutController::dispatchAction(const QString &portalId)
{
    switch (AppShortcuts::actionFromPortalId(portalId)) {
    case AppShortcuts::PlayPause:
        emit playPauseTriggered();
        break;
    case AppShortcuts::NextTrack:
        emit nextTrackTriggered();
        break;
    case AppShortcuts::PreviousTrack:
        emit previousTrackTriggered();
        break;
    default:
        break;
    }
}
