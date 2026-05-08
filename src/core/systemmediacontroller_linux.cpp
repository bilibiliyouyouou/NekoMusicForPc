#include "systemmediacontroller.h"
#include "core/playerengine.h"
#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusError>
#include <QDebug>
#include <QTimer>
#include <QWidget>
#include <QUrl>

class MprisRootAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2")
    Q_PROPERTY(bool CanQuit READ canQuit CONSTANT)
    Q_PROPERTY(bool FullScreen READ fullScreen CONSTANT)
    Q_PROPERTY(bool CanSetFullScreen READ canSetFullScreen CONSTANT)
    Q_PROPERTY(bool CanRaise READ canRaise CONSTANT)
    Q_PROPERTY(bool HasTrackList READ hasTrackList CONSTANT)
    Q_PROPERTY(QString Identity READ identity CONSTANT)
    Q_PROPERTY(QString DesktopEntry READ desktopEntry CONSTANT)
    Q_PROPERTY(QStringList SupportedUriSchemes READ supportedUriSchemes CONSTANT)
    Q_PROPERTY(QStringList SupportedMimeTypes READ supportedMimeTypes CONSTANT)

public:
    explicit MprisRootAdaptor(SystemMediaController *parent);

    bool canQuit() const;
    bool fullScreen() const;
    bool canSetFullScreen() const;
    bool canRaise() const;
    bool hasTrackList() const;
    QString identity() const;
    QString desktopEntry() const;
    QStringList supportedUriSchemes() const;
    QStringList supportedMimeTypes() const;

public slots:
    void Raise();
    void Quit();

private:
    SystemMediaController *m_ctrl;
};

class MprisPlayerAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player")
    Q_PROPERTY(QString PlaybackStatus READ playbackStatus NOTIFY playbackStatusChanged)
    Q_PROPERTY(QString LoopStatus READ loopStatus NOTIFY loopStatusChanged)
    Q_PROPERTY(double Rate READ rate CONSTANT)
    Q_PROPERTY(bool Shuffle READ shuffle NOTIFY shuffleChanged)
    Q_PROPERTY(QVariantMap Metadata READ metadata NOTIFY metadataChanged)
    Q_PROPERTY(double Volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(qlonglong Position READ position NOTIFY positionChanged)
    Q_PROPERTY(double MinimumRate READ minimumRate CONSTANT)
    Q_PROPERTY(double MaximumRate READ maximumRate CONSTANT)
    Q_PROPERTY(bool CanGoNext READ canGoNext NOTIFY canGoNextChanged)
    Q_PROPERTY(bool CanGoPrevious READ canGoPrevious NOTIFY canGoPreviousChanged)
    Q_PROPERTY(bool CanPlay READ canPlay NOTIFY canPlayChanged)
    Q_PROPERTY(bool CanPause READ canPause NOTIFY canPauseChanged)
    Q_PROPERTY(bool CanSeek READ canSeek NOTIFY canSeekChanged)
    Q_PROPERTY(bool CanControl READ canControl CONSTANT)

public:
    explicit MprisPlayerAdaptor(SystemMediaController *parent);

    QString playbackStatus() const;
    QString loopStatus() const;
    double rate() const;
    bool shuffle() const;
    QVariantMap metadata() const;
    double volume() const;
    void setVolume(double v);
    qlonglong position() const;
    double minimumRate() const;
    double maximumRate() const;
    bool canGoNext() const;
    bool canGoPrevious() const;
    bool canPlay() const;
    bool canPause() const;
    bool canSeek() const;
    bool canControl() const;

public slots:
    void Next();
    void Previous();
    void Pause();
    void PlayPause();
    void Stop();
    void Play();
    void Seek(qlonglong offset);
    void SetPosition(const QDBusObjectPath &trackId, qlonglong position);

    void tellPlaybackStatusChanged() { emit playbackStatusChanged(); }
    void tellLoopStatusChanged() { emit loopStatusChanged(); }
    void tellShuffleChanged() { emit shuffleChanged(); }
    void tellMetadataChanged() { emit metadataChanged(); }
    void tellVolumeChanged() { emit volumeChanged(); }
    void tellPositionChanged() { emit positionChanged(); }
    void tellCanGoNextChanged() { emit canGoNextChanged(); }
    void tellCanGoPreviousChanged() { emit canGoPreviousChanged(); }
    void tellCanPlayChanged() { emit canPlayChanged(); }
    void tellCanPauseChanged() { emit canPauseChanged(); }
    void tellCanSeekChanged() { emit canSeekChanged(); }

signals:
    void playbackStatusChanged();
    void loopStatusChanged();
    void shuffleChanged();
    void metadataChanged();
    void volumeChanged();
    void positionChanged();
    void canGoNextChanged();
    void canGoPreviousChanged();
    void canPlayChanged();
    void canPauseChanged();
    void canSeekChanged();

private:
    SystemMediaController *m_ctrl;
};

namespace {
constexpr char kMprisPath[] = "/org/mpris/MediaPlayer2";
constexpr char kMprisService[] = "org.mpris.MediaPlayer2.NekoMusic";
constexpr int kPositionEmitIntervalMs = 900;
}

MprisRootAdaptor::MprisRootAdaptor(SystemMediaController *parent)
    : QDBusAbstractAdaptor(parent)
    , m_ctrl(parent)
{
}

bool MprisRootAdaptor::canQuit() const { return true; }
bool MprisRootAdaptor::fullScreen() const { return false; }
bool MprisRootAdaptor::canSetFullScreen() const { return false; }
bool MprisRootAdaptor::canRaise() const { return true; }
bool MprisRootAdaptor::hasTrackList() const { return false; }

QString MprisRootAdaptor::identity() const
{
    return QStringLiteral("Neko云音乐");
}

QString MprisRootAdaptor::desktopEntry() const
{
    return QStringLiteral("nekomusic");
}

QStringList MprisRootAdaptor::supportedUriSchemes() const
{
    return {QStringLiteral("http"), QStringLiteral("https"), QStringLiteral("file")};
}

QStringList MprisRootAdaptor::supportedMimeTypes() const
{
    return {QStringLiteral("audio/mpeg"), QStringLiteral("audio/flac"), QStringLiteral("audio/ogg"),
            QStringLiteral("audio/x-wav")};
}

void MprisRootAdaptor::Raise()
{
    m_ctrl->fireRaise();
}

void MprisRootAdaptor::Quit()
{
    m_ctrl->fireQuit();
}

MprisPlayerAdaptor::MprisPlayerAdaptor(SystemMediaController *parent)
    : QDBusAbstractAdaptor(parent)
    , m_ctrl(parent)
{
}

QString MprisPlayerAdaptor::playbackStatus() const { return m_ctrl->mprisPlaybackStatus(); }
QString MprisPlayerAdaptor::loopStatus() const { return m_ctrl->mprisLoopStatus(); }
double MprisPlayerAdaptor::rate() const { return 1.0; }
bool MprisPlayerAdaptor::shuffle() const { return m_ctrl->mprisShuffle(); }
QVariantMap MprisPlayerAdaptor::metadata() const { return m_ctrl->mprisMetadata(); }
double MprisPlayerAdaptor::volume() const { return m_ctrl->mprisVolume(); }

void MprisPlayerAdaptor::setVolume(double v)
{
    m_ctrl->applyVolumeFromMpris(v);
}

qlonglong MprisPlayerAdaptor::position() const
{
    return m_ctrl->mprisPositionUs();
}

double MprisPlayerAdaptor::minimumRate() const { return 1.0; }
double MprisPlayerAdaptor::maximumRate() const { return 1.0; }
bool MprisPlayerAdaptor::canGoNext() const { return m_ctrl->mprisCanGoNext(); }
bool MprisPlayerAdaptor::canGoPrevious() const { return m_ctrl->mprisCanGoPrevious(); }
bool MprisPlayerAdaptor::canPlay() const { return m_ctrl->mprisCanPlay(); }
bool MprisPlayerAdaptor::canPause() const { return m_ctrl->mprisCanPause(); }
bool MprisPlayerAdaptor::canSeek() const { return m_ctrl->mprisCanSeek(); }
bool MprisPlayerAdaptor::canControl() const { return true; }

void MprisPlayerAdaptor::Next()
{
    m_ctrl->fireNext();
}

void MprisPlayerAdaptor::Previous()
{
    m_ctrl->firePrevious();
}

void MprisPlayerAdaptor::Pause()
{
    m_ctrl->firePause();
}

void MprisPlayerAdaptor::PlayPause()
{
    m_ctrl->firePlayPause();
}

void MprisPlayerAdaptor::Stop()
{
    m_ctrl->fireStop();
}

void MprisPlayerAdaptor::Play()
{
    m_ctrl->firePlay();
}

void MprisPlayerAdaptor::Seek(qlonglong offset)
{
    m_ctrl->fireSeekRelative(offset);
}

void MprisPlayerAdaptor::SetPosition(const QDBusObjectPath &trackId, qlonglong position)
{
    if (trackId.path() != m_ctrl->mprisCurrentTrackPath())
        return;
    m_ctrl->fireSeekAbsolute(position);
}

SystemMediaController::SystemMediaController(QObject *parent)
    : QObject(parent)
    , m_rootAdaptor(new MprisRootAdaptor(this))
    , m_playerAdaptor(new MprisPlayerAdaptor(this))
{
    Q_UNUSED(m_rootAdaptor);

    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        qWarning() << "MPRIS: D-Bus session unavailable, system media keys integration disabled";
        return;
    }

    if (!bus.registerService(QString::fromLatin1(kMprisService))) {
        qWarning() << "MPRIS: registerService failed:" << bus.lastError().message();
        return;
    }

    if (!bus.registerObject(QString::fromLatin1(kMprisPath), this, QDBusConnection::ExportAdaptors)) {
        qWarning() << "MPRIS: registerObject failed:" << bus.lastError().message();
        bus.unregisterService(QString::fromLatin1(kMprisService));
        return;
    }
}

SystemMediaController::~SystemMediaController()
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (bus.isConnected()) {
        bus.unregisterObject(QString::fromLatin1(kMprisPath));
        bus.unregisterService(QString::fromLatin1(kMprisService));
    }
}

void SystemMediaController::setHostWindow(QWidget *host)
{
    m_hostWindow = host;
    Q_UNUSED(m_hostWindow);
}

void SystemMediaController::setPlayerEngine(PlayerEngine *engine)
{
    m_engine = engine;
}

void SystemMediaController::updateFromEngineState(PlayerEngine::PlaybackState state)
{
    QString next;
    switch (state) {
    case PlayerEngine::Playing:
        next = QStringLiteral("Playing");
        break;
    case PlayerEngine::Paused:
        next = QStringLiteral("Paused");
        break;
    default:
        next = QStringLiteral("Stopped");
        break;
    }
    if (m_playbackStatus == next) {
        if (state == PlayerEngine::Playing)
            startPositionTimer();
        else
            stopPositionTimer();
        // PlaybackStatus 未变但底层 QMediaPlayer 与 m_state 曾脱节时，CanPlay/CanPause 仍需刷新（KDE 等会读）
        m_playerAdaptor->tellCanPlayChanged();
        m_playerAdaptor->tellCanPauseChanged();
        return;
    }
    m_playbackStatus = next;
    m_playerAdaptor->tellPlaybackStatusChanged();
    m_playerAdaptor->tellCanPlayChanged();
    m_playerAdaptor->tellCanPauseChanged();

    if (state == PlayerEngine::Playing)
        startPositionTimer();
    else
        stopPositionTimer();
}

void SystemMediaController::updateMetadata(const MusicInfo &music, qint64 durationMs)
{
    QVariantMap meta;
    if (music.id > 0) {
        const QString trackPath = QStringLiteral("/org/mpris/MediaPlayer2/track/%1").arg(music.id);
        m_currentTrackId = QDBusObjectPath(trackPath);
        meta.insert(QStringLiteral("mpris:trackid"), QVariant::fromValue(m_currentTrackId));
        meta.insert(QStringLiteral("xesam:title"), music.title);
        if (!music.artist.isEmpty())
            meta.insert(QStringLiteral("xesam:artist"), QStringList{music.artist});
        if (!music.album.isEmpty())
            meta.insert(QStringLiteral("xesam:album"), music.album);
        if (!music.coverUrl.isEmpty())
            meta.insert(QStringLiteral("mpris:artUrl"), music.coverUrl);
        const qint64 lenMs = qMax(durationMs, static_cast<qint64>(music.duration) * 1000);
        if (lenMs > 0)
            meta.insert(QStringLiteral("mpris:length"), lenMs * 1000);
    } else if (music.isLocalFile()) {
        const QString trackPath = QStringLiteral("/org/mpris/MediaPlayer2/track/local%1")
            .arg(qHash(music.localPath));
        m_currentTrackId = QDBusObjectPath(trackPath);
        meta.insert(QStringLiteral("mpris:trackid"), QVariant::fromValue(m_currentTrackId));
        meta.insert(QStringLiteral("xesam:title"), music.title);
        if (!music.artist.isEmpty())
            meta.insert(QStringLiteral("xesam:artist"), QStringList{music.artist});
        if (!music.album.isEmpty())
            meta.insert(QStringLiteral("xesam:album"), music.album);
        if (!music.coverUrl.isEmpty())
            meta.insert(QStringLiteral("mpris:artUrl"), music.coverUrl);
        meta.insert(QStringLiteral("xesam:url"), QUrl::fromLocalFile(music.localPath).toString());
        const qint64 lenMs = qMax(durationMs, static_cast<qint64>(music.duration) * 1000);
        if (lenMs > 0)
            meta.insert(QStringLiteral("mpris:length"), lenMs * 1000);
    } else {
        m_currentTrackId = QDBusObjectPath();
    }

    m_metadata = meta;
    m_playerAdaptor->tellMetadataChanged();
    m_playerAdaptor->tellPositionChanged();
    m_lastPositionEmitMs = -1;
}

void SystemMediaController::updateCapabilities(bool canNext, bool canPrev, bool canSeek)
{
    if (m_canGoNext != canNext) {
        m_canGoNext = canNext;
        m_playerAdaptor->tellCanGoNextChanged();
    }
    if (m_canGoPrevious != canPrev) {
        m_canGoPrevious = canPrev;
        m_playerAdaptor->tellCanGoPreviousChanged();
    }
    if (m_canSeek != canSeek) {
        m_canSeek = canSeek;
        m_playerAdaptor->tellCanSeekChanged();
    }
}

void SystemMediaController::updateLoopShuffle(const QString &playMode)
{
    QString loop = QStringLiteral("None");
    bool shuf = false;
    if (playMode == QStringLiteral("single"))
        loop = QStringLiteral("Track");
    else if (playMode == QStringLiteral("list"))
        loop = QStringLiteral("Playlist");
    else if (playMode == QStringLiteral("random"))
        shuf = true;

    if (m_loopStatus != loop) {
        m_loopStatus = loop;
        m_playerAdaptor->tellLoopStatusChanged();
    }
    if (m_shuffle != shuf) {
        m_shuffle = shuf;
        m_playerAdaptor->tellShuffleChanged();
    }
}

void SystemMediaController::syncVolumeFromEngine(double volume01)
{
    volume01 = qBound(0.0, volume01, 1.0);
    if (qFuzzyCompare(m_volume + 1.0, volume01 + 1.0))
        return;
    m_volume = volume01;
    m_playerAdaptor->tellVolumeChanged();
}

void SystemMediaController::onPositionMsChanged(qint64 positionMs)
{
    Q_UNUSED(positionMs);
    if (m_playbackStatus != QStringLiteral("Playing"))
        return;
    if (!m_engine)
        return;
    const qint64 ms = m_engine->position();
    if (m_lastPositionEmitMs < 0 || qAbs(ms - m_lastPositionEmitMs) >= 1000) {
        m_lastPositionEmitMs = ms;
        m_playerAdaptor->tellPositionChanged();
    }
}

void SystemMediaController::startPositionTimer()
{
    if (!m_positionTimer) {
        m_positionTimer = new QTimer(this);
        m_positionTimer->setInterval(kPositionEmitIntervalMs);
        connect(m_positionTimer, &QTimer::timeout, this, [this]() {
            if (!m_engine || m_playbackStatus != QStringLiteral("Playing"))
                return;
            const qint64 ms = m_engine->position();
            if (m_lastPositionEmitMs < 0 || qAbs(ms - m_lastPositionEmitMs) >= 1000) {
                m_lastPositionEmitMs = ms;
                m_playerAdaptor->tellPositionChanged();
            }
        });
    }
    m_lastPositionEmitMs = -1;
    m_positionTimer->start();
}

void SystemMediaController::stopPositionTimer()
{
    if (m_positionTimer)
        m_positionTimer->stop();
}

void SystemMediaController::applyVolumeFromMpris(double v)
{
    v = qBound(0.0, v, 1.0);
    if (qFuzzyCompare(m_volume + 1.0, v + 1.0))
        return;
    m_volume = v;
    m_playerAdaptor->tellVolumeChanged();
    emit volumeSetByOs(v);
}

qlonglong SystemMediaController::mprisPositionUs() const
{
    if (!m_engine)
        return 0;
    return static_cast<qlonglong>(m_engine->position()) * 1000LL;
}

bool SystemMediaController::mprisCanPlay() const
{
    if (!m_engine)
        return false;
    // Keep capability evaluation consistent with exported PlaybackStatus to avoid
    // transient state drift in desktop media UIs.
    return m_playbackStatus != QStringLiteral("Playing");
}

bool SystemMediaController::mprisCanPause() const
{
    if (!m_engine)
        return false;
    return m_playbackStatus == QStringLiteral("Playing");
}

#include "systemmediacontroller_linux.moc"
