/**
 * @file main.cpp
 * @brief NekoMusic 日系动漫风入口
 */

#include <QApplication>
#include <QDebug>
#include <QSettings>
#include <QLocalSocket>
#include <QLocalServer>
#include <QTimer>
#include <QFileInfo>
#include "ui/mainwindow.h"
#include "core/i18n.h"
#include "core/playlistdb.h"
#include "core/localmusicmeta.h"
#include "version.h"

// 单实例服务器名称
static const QString kServerName = QStringLiteral("NekoMusicSingleInstance");

static QString firstLaunchAudioPath(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        const QString a = QString::fromUtf8(argv[i]);
        if (a.startsWith(QLatin1Char('-')))
            continue;
        const QString local = LocalMusic::normalizeOpenPathArgument(a);
        if (local.isEmpty())
            continue;
        QFileInfo fi(local);
        if (!fi.exists() || !fi.isFile())
            continue;
        const QString abs = fi.canonicalFilePath().isEmpty() ? fi.absoluteFilePath() : fi.canonicalFilePath();
        if (LocalMusic::isSupportedLocalAudioFile(abs))
            return abs;
    }
    return {};
}

int main(int argc, char *argv[])
{
#ifdef Q_OS_LINUX
    // Linux: 使用 PulseAudio 后端避免 PipeWire 初始化阻塞
    qputenv("QT_MULTIMEDIA_BACKEND", "pulse");
#endif

    // 检查是否已有实例在运行
    QLocalSocket socket;
    socket.connectToServer(kServerName);
    if (socket.waitForConnected(3000)) {
        const QString audio = firstLaunchAudioPath(argc, argv);
        if (!audio.isEmpty()) {
            QByteArray msg = QByteArrayLiteral("PLAY\t") + audio.toUtf8() + '\n';
            socket.write(msg);
        } else {
            socket.write("SHOW\n");
        }
        socket.flush();
        socket.waitForBytesWritten(3000);
        return 0;
    }

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("NekoMusic"));
    app.setApplicationVersion(QStringLiteral(APP_VERSION));
    app.setOrganizationName(QStringLiteral("NekoMusic"));
    app.setOrganizationDomain(QStringLiteral("nekomusic.local"));

    // 加载用户设置
    QSettings settings;
    int lang = settings.value("language", static_cast<int>(I18n::ZhCN)).toInt();
    I18n::instance().setLanguage(static_cast<I18n::Language>(lang));

    // 初始化播放列表数据库
    PlaylistDatabase::instance().init();

    QFont font(QStringLiteral("Segoe UI"), 14);
    font.setStyleHint(QFont::SansSerif);
    app.setFont(font);
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/app.png")));

    QApplication::setQuitOnLastWindowClosed(false);

    MainWindow *window = new MainWindow;

    // 创建单实例服务器
    QLocalServer::removeServer(kServerName);
    QLocalServer *server = new QLocalServer(&app);
    if (!server->listen(kServerName)) {
        qWarning() << "NekoMusic: single-instance socket listen failed:" << server->errorString()
                   << "- external open may spawn duplicate processes.";
    } else {
        QObject::connect(server, &QLocalServer::newConnection, [server, window]() {
            QLocalSocket *clientSocket = server->nextPendingConnection();
            auto *buf = new QByteArray;

            auto processLine = [window](const QByteArray &raw) {
                QByteArray line = raw;
                line = line.trimmed();
                if (line.isEmpty())
                    return;
                static const QByteArray kPlayPfx = QByteArrayLiteral("PLAY\t");
                if (line.startsWith(kPlayPfx)) {
                    QString path = QString::fromUtf8(line.mid(kPlayPfx.size())).trimmed();
                    path = LocalMusic::normalizeOpenPathArgument(path);
                    window->show();
                    window->raise();
                    window->activateWindow();
                    if (!path.isEmpty()) {
                        QTimer::singleShot(0, window, [window, path]() {
                            window->openAudioFileFromPath(path);
                        });
                    }
                } else if (line == "SHOW") {
                    window->show();
                    window->raise();
                    window->activateWindow();
                }
            };

            QObject::connect(clientSocket, &QLocalSocket::readyRead, clientSocket, [clientSocket, window, buf, processLine]() {
                buf->append(clientSocket->readAll());
                while (true) {
                    const int nl = buf->indexOf('\n');
                    if (nl < 0)
                        break;
                    QByteArray line = buf->left(nl);
                    buf->remove(0, nl + 1);
                    processLine(line);
                }
            });
            QObject::connect(clientSocket, &QLocalSocket::disconnected, clientSocket, [clientSocket, buf, processLine]() {
                if (!buf->isEmpty())
                    processLine(*buf);
                delete buf;
                clientSocket->deleteLater();
            });
        });
    }

    window->show();

    const QString launchAudio = firstLaunchAudioPath(argc, argv);
    if (!launchAudio.isEmpty()) {
        QTimer::singleShot(0, window, [window, launchAudio]() { window->openAudioFileFromPath(launchAudio); });
    }

    int result = app.exec();

    // 清理数据库
    PlaylistDatabase::instance().close();

    return result;
}
