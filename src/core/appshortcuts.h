#pragma once

#include <QObject>
#include <QKeySequence>

/** 播放快捷键：持久化到 QSettings；全局注册走 xdg-desktop-portal。 */
class AppShortcuts final : public QObject
{
    Q_OBJECT

public:
    enum Action {
        PlayPause = 0,
        NextTrack,
        PreviousTrack,
        ActionCount
    };
    Q_ENUM(Action)

    static AppShortcuts &instance();

    QKeySequence sequence(Action action) const;
    void setSequence(Action action, const QKeySequence &seq);
    void resetToDefaults();
    void resetAllAndSave();
    void load();
    void save();

    static QKeySequence defaultSequence(Action action);
    static QString settingsKey(Action action);
    static QString portalShortcutId(Action action);
    static QString actionLabel(Action action);
    static QString toPortalTrigger(const QKeySequence &seq);
    static AppShortcuts::Action actionFromPortalId(const QString &id);

signals:
    void shortcutsChanged();

private:
    explicit AppShortcuts(QObject *parent = nullptr);

    QKeySequence m_sequences[ActionCount];
};
