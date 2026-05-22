/**
 * @file apiclient.cpp
 * @brief API 客户端实现
 */

#include "apiclient.h"
#include "httpprotocollabel.h"
#include "theme/theme.h"
#include "core/usermanager.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QSaveFile>

ApiClient::ApiClient(QObject *parent) : QObject(parent) {}

// 现有函数
void ApiClient::fetchRanking(MusicListCb cb) {
    QUrl url(QString::fromUtf8("%1/api/music/ranking")
                 .arg(QString::fromUtf8(Theme::kApiBase)));
    auto *reply = m_nam.get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) { cb(false, {}); return; }
        auto doc = QJsonDocument::fromJson(reply->readAll());
        bool ok = doc.object().value("success").toBool();
        QList<QVariantMap> res;
        if (ok) for (const auto &v : doc.object().value("data").toArray())
            res.append(v.toObject().toVariantMap());
        cb(ok, res);
    });
}

void ApiClient::fetchLatest(int limit, MusicListCb cb) {
    QUrl url(QString::fromUtf8("%1/api/music/latest?limit=%2")
                 .arg(QString::fromUtf8(Theme::kApiBase))
                 .arg(limit));
    auto *reply = m_nam.get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) { cb(false, {}); return; }
        auto doc = QJsonDocument::fromJson(reply->readAll());
        bool ok = doc.object().value("success").toBool();
        QList<QVariantMap> res;
        if (ok) for (const auto &v : doc.object().value("data").toArray())
            res.append(v.toObject().toVariantMap());
        cb(ok, res);
    });
}

void ApiClient::fetchFavorites(MusicListCb cb) {
    QUrl url(QString::fromUtf8("%1/api/user/favorites").arg(Theme::kApiBase));
    QNetworkRequest req(url);
    if (UserManager::instance().isLoggedIn()) {
        req.setRawHeader("Authorization", UserManager::instance().token().toUtf8());
    }
    auto *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) { cb(false, {}); return; }
        auto doc = QJsonDocument::fromJson(reply->readAll());
        bool ok = doc.object().value("success").toBool();
        QList<QVariantMap> res;
        if (ok) for (const auto &v : doc.object().value("favorites").toArray())
            res.append(v.toObject().toVariantMap());
        cb(ok, res);
    });
}

void ApiClient::login(const QString &username, const QString &password, AuthCb cb) {
    QUrl url(QString::fromUtf8("%1/api/user/login").arg(Theme::kApiBase));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject body;
    body["username"] = username;
    body["password"] = password;
    auto *reply = m_nam.post(req, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            cb(false, reply->errorString(), {}, {});
            return;
        }
        auto doc = QJsonDocument::fromJson(reply->readAll());
        bool ok = doc.object().value("success").toBool();
        QString message = doc.object().value("message").toString();
        QString token;
        QVariantMap user;
        if (ok) {
            auto data = doc.object().value("data").toObject();
            token = data.value("token").toString();
            user = data.value("user").toObject().toVariantMap();
        }
        cb(ok, message, token, user);
    });
}

void ApiClient::registerUser(const QString &username, const QString &password,
                              const QString &email, const QString &verificationCode, AuthCb cb) {
    QUrl url(QString::fromUtf8("%1/api/user/register").arg(Theme::kApiBase));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject body;
    body["username"] = username;
    body["password"] = password;
    body["email"] = email;
    body["verificationCode"] = verificationCode;
    auto *reply = m_nam.post(req, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            cb(false, reply->errorString(), {}, {});
            return;
        }
        auto doc = QJsonDocument::fromJson(reply->readAll());
        bool ok = doc.object().value("success").toBool();
        QString message = doc.object().value("message").toString();
        QString token;
        QVariantMap user;
        if (ok) {
            auto data = doc.object().value("data").toObject();
            token = data.value("token").toString();
            user = data.value("user").toObject().toVariantMap();
        }
        cb(ok, message, token, user);
    });
}

void ApiClient::sendVerificationCode(const QString &email, const QString &username,
                                     const QString &captchaPassToken,
                                     std::function<void(bool, const QString &)> cb) {
    QUrl url(QString::fromUtf8("%1/api/user/send-verification").arg(Theme::kApiBase));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject body;
    body["email"] = email;
    body["username"] = username;
    body["captchaPassToken"] = captchaPassToken;
    auto *reply = m_nam.post(req, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        const QByteArray raw = reply->readAll();
        auto doc = QJsonDocument::fromJson(raw);
        const QString message = doc.object().value("message").toString();
        if (reply->error() != QNetworkReply::NoError) {
            cb(false, message.isEmpty() ? reply->errorString() : message);
            return;
        }
        const bool ok = doc.object().value("success").toBool();
        cb(ok, message);
    });
}

void ApiClient::fetchSliderCaptchaChallenge(SliderCaptchaChallengeCb cb) {
    QUrl url(QString::fromUtf8("%1/api/captcha/slider").arg(Theme::kApiBase));
    auto *reply = m_nam.get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        const QByteArray raw = reply->readAll();
        auto doc = QJsonDocument::fromJson(raw);
        const QString msg = doc.object().value("message").toString();
        if (reply->error() != QNetworkReply::NoError) {
            cb(false, msg.isEmpty() ? reply->errorString() : msg, {});
            return;
        }
        const bool ok = doc.object().value("success").toBool();
        const QVariantMap data = doc.object().value("data").toObject().toVariantMap();
        cb(ok, msg, data);
    });
}

void ApiClient::verifySliderCaptcha(const QString &captchaToken, int captchaOffsetX, SliderCaptchaVerifyCb cb) {
    QUrl url(QString::fromUtf8("%1/api/captcha/slider/verify").arg(Theme::kApiBase));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject body;
    body["captchaToken"] = captchaToken;
    body["captchaOffsetX"] = captchaOffsetX;
    auto *reply = m_nam.post(req, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        const QByteArray raw = reply->readAll();
        auto doc = QJsonDocument::fromJson(raw);
        const QString msg = doc.object().value("message").toString();
        if (reply->error() != QNetworkReply::NoError) {
            cb(false, msg.isEmpty() ? reply->errorString() : msg, {});
            return;
        }
        const bool ok = doc.object().value("success").toBool();
        QString pass;
        if (ok) {
            const auto data = doc.object().value("data").toObject();
            pass = data.value("captchaPassToken").toString();
        }
        cb(ok, msg, pass);
    });
}

void ApiClient::sendResetCode(const QString &email, std::function<void(bool, const QString&)> cb) {
    QUrl url(QString::fromUtf8("%1/api/user/send-reset-code").arg(Theme::kApiBase));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject body;
    body["email"] = email;
    auto *reply = m_nam.post(req, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            cb(false, reply->errorString());
            return;
        }
        auto doc = QJsonDocument::fromJson(reply->readAll());
        bool ok = doc.object().value("success").toBool();
        QString message = doc.object().value("message").toString();
        cb(ok, message);
    });
}

void ApiClient::resetPassword(const QString &email, const QString &verificationCode,
                               const QString &newPassword, std::function<void(bool, const QString&)> cb) {
    QUrl url(QString::fromUtf8("%1/api/user/reset-password").arg(Theme::kApiBase));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject body;
    body["email"] = email;
    body["verificationCode"] = verificationCode;
    body["newPassword"] = newPassword;
    auto *reply = m_nam.post(req, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            cb(false, reply->errorString());
            return;
        }
        auto doc = QJsonDocument::fromJson(reply->readAll());
        bool ok = doc.object().value("success").toBool();
        QString message = doc.object().value("message").toString();
        cb(ok, message);
    });
}

// 新实现的API
void ApiClient::searchMusic(const QString &query, int page, int pageSize, MusicSearchCb cb) {
    QUrl url(QString::fromUtf8("%1/api/music/search").arg(Theme::kApiBase));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject body;
    body["query"] = query;
    auto *reply = m_nam.post(req, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [reply, cb, query]() {
        reply->deleteLater();
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "[搜索API]搜索:" << query << "，HTTP状态码:" << statusCode
                     << "，协议:" << httpProtocolLabel(reply) << "，错误:" << reply->errorString();
            if (cb) cb(false, 0, 0, 0, {});
            return;
        }
        auto response = reply->readAll();
        auto doc = QJsonDocument::fromJson(response);
        bool ok = doc.object().value("success").toBool();
        QList<QVariantMap> results;
        int total = 0, currentPage = 0, currentPageSize = 0;
        if (ok) {
            auto obj = doc.object();
            // Support both formats: with or without "data" wrapper
            if (obj.contains("data")) {
                auto data = obj.value("data").toObject();
                total = data.value("total").toInt();
                currentPage = data.value("page").toInt();
                currentPageSize = data.value("pageSize").toInt();
                for (const auto &v : data.value("results").toArray()) {
                    results.append(v.toObject().toVariantMap());
                }
            } else {
                total = obj.value("total").toInt();
                currentPage = 1;
                currentPageSize = obj.value("results").toArray().size();
                for (const auto &v : obj.value("results").toArray()) {
                    results.append(v.toObject().toVariantMap());
                }
            }
            qDebug() << "[搜索API]搜索:" << query << "，HTTP状态码:" << statusCode
                     << "，协议:" << httpProtocolLabel(reply) << "，成功，找到" << total << "个结果";
        } else {
            QString message = doc.object().value("message").toString();
            qDebug() << "[搜索API]搜索:" << query << "，HTTP状态码:" << statusCode
                     << "，协议:" << httpProtocolLabel(reply) << "，失败，消息:" << message;
        }
        if (cb) cb(ok, total, currentPage, currentPageSize, results);
    });
}

void ApiClient::fetchMusicInfo(int musicId, MusicInfoCb cb) {
    QUrl url(QString::fromUtf8("%1/api/music/info/%2").arg(Theme::kApiBase).arg(musicId));
    auto *reply = m_nam.get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            if (cb) cb(false, {});
            return;
        }
        auto doc = QJsonDocument::fromJson(reply->readAll());
        bool ok = doc.object().value("success").toBool();
        QVariantMap musicInfo;
        if (ok) {
            auto data = doc.object().value("data").toObject();
            musicInfo = data.toVariantMap();
        }
        if (cb) cb(ok, musicInfo);
    });
}

void ApiClient::fetchLyrics(int musicId, LyricsCb cb) {
    QUrl url(QString::fromUtf8("%1/api/music/lyrics/%2")
                 .arg(QString::fromUtf8(Theme::kApiBase))
                 .arg(musicId));
    auto *reply = m_nam.get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            if (cb) cb(false, "");
            return;
        }
        auto doc = QJsonDocument::fromJson(reply->readAll());
        bool ok = doc.object().value("success").toBool();
        QString lyrics = "";
        if (ok) {
            lyrics = doc.object().value("data").toString();
        }
        if (cb) cb(ok, lyrics);
    });
}

void ApiClient::searchArtists(const QString &query, ArtistSearchCb cb) {
    QUrl url(QString::fromUtf8("%1/api/artists/search").arg(Theme::kApiBase));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject body;
    body["query"] = query;
    auto *reply = m_nam.post(req, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            if (cb) cb(false, {});
            return;
        }
        auto doc = QJsonDocument::fromJson(reply->readAll());
        bool ok = doc.object().value("success").toBool();
        QVariantMap artistInfo;
        if (ok) {
            artistInfo = doc.object().value("artist").toObject().toVariantMap();
        }
        if (cb) cb(ok, artistInfo);
    });
}

void ApiClient::fetchUploadedMusic(UploadedMusicCb cb) {
    QUrl url(QString::fromUtf8("%1/api/user/uploaded-music").arg(Theme::kApiBase));
    QNetworkRequest req(url);
    if (UserManager::instance().isLoggedIn()) {
        req.setRawHeader("Authorization", UserManager::instance().token().toUtf8());
    }
    auto *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            if (cb) cb(false, 0, {});
            return;
        }
        auto doc = QJsonDocument::fromJson(reply->readAll());
        bool ok = doc.object().value("success").toBool();
        QList<QVariantMap> musicList;
        int total = 0;
        if (ok) {
            total = doc.object().value("total").toInt();
            for (const auto &v : doc.object().value("musicList").toArray()) {
                musicList.append(v.toObject().toVariantMap());
            }
        }
        if (cb) cb(ok, total, musicList);
    });
}

void ApiClient::changePassword(const QString &oldPassword, const QString &newPassword, std::function<void(bool, const QString&)> cb) {
    QUrl url(QString::fromUtf8("%1/api/user/password/change").arg(Theme::kApiBase));
    QNetworkRequest req(url);
    if (UserManager::instance().isLoggedIn()) {
        req.setRawHeader("Authorization", QString("Bearer %1").arg(UserManager::instance().token()).toUtf8());
    }
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject body;
    body["oldPassword"] = oldPassword;
    body["newPassword"] = newPassword;
    auto *reply = m_nam.post(req, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            if (cb) cb(false, reply->errorString());
            return;
        }
        auto doc = QJsonDocument::fromJson(reply->readAll());
        bool ok = doc.object().value("success").toBool();
        QString message = ok ? doc.object().value("message").toString() : doc.object().value("error").toString();
        if (cb) cb(ok, message);
    });
}

void ApiClient::fetchPlaylists(const QString &query, PlaylistsCb cb) {
    QUrl url(QString::fromUtf8("%1/api/playlists/search").arg(Theme::kApiBase));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject body;
    body["query"] = query;
    auto *reply = m_nam.post(req, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) { cb(false, {}); return; }
        auto doc = QJsonDocument::fromJson(reply->readAll());
        bool ok = doc.object().value("success").toBool();
        QList<QVariantMap> res;
        if (ok) for (const auto &v : doc.object().value("results").toArray())
            res.append(v.toObject().toVariantMap());
        cb(ok, res);
    });
}

void ApiClient::fetchPlaylistDetail(int playlistId, PlaylistDetailCb cb) {
    QUrl url(QString::fromUtf8("%1/api/playlist/%2").arg(Theme::kApiBase).arg(playlistId));
    auto *reply = m_nam.get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            if (cb) cb(false, {});
            return;
        }
        auto doc = QJsonDocument::fromJson(reply->readAll());
        bool ok = doc.object().value("success").toBool();
        QVariantMap detail;
        if (ok) {
            detail = doc.object().value("playlist").toObject().toVariantMap();
        }
        if (cb) cb(ok, detail);
    });
}

void ApiClient::fetchUserPlaylists(UserPlaylistsCb cb) {
    QUrl url(QString::fromUtf8("%1/api/user/playlists").arg(Theme::kApiBase));
    QNetworkRequest req(url);
    if (UserManager::instance().isLoggedIn()) {
        req.setRawHeader("Authorization", UserManager::instance().token().toUtf8());
    }
    auto *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            if (cb) cb(false, {});
            return;
        }
        auto doc = QJsonDocument::fromJson(reply->readAll());
        bool ok = doc.object().value("success").toBool();
        QList<QVariantMap> res;
        if (ok) for (const auto &v : doc.object().value("playlists").toArray())
            res.append(v.toObject().toVariantMap());
        if (cb) cb(ok, res);
    });
}

void ApiClient::createPlaylist(const QString &name, const QString &description, CreatePlaylistCb cb) {
    QUrl url(QString::fromUtf8("%1/api/user/playlist/create").arg(Theme::kApiBase));
    QNetworkRequest req(url);
    req.setRawHeader("Authorization", UserManager::instance().token().toUtf8());
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject body;
    body["name"] = name;
    if (!description.isEmpty()) body["description"] = description;
    auto *reply = m_nam.post(req, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            if (cb) cb(false, reply->errorString(), {});
            return;
        }
        auto doc = QJsonDocument::fromJson(reply->readAll());
        bool ok = doc.object().value("success").toBool();
        QString message = doc.object().value("message").toString();
        QVariantMap playlist;
        if (ok) {
            playlist = doc.object().value("playlist").toObject().toVariantMap();
        }
        if (cb) cb(ok, message, playlist);
    });
}

void ApiClient::updatePlaylist(int playlistId, const QString &name, const QString &description, UpdatePlaylistCb cb) {
    QUrl url(QString::fromUtf8("%1/api/user/playlist/update").arg(Theme::kApiBase));
    QNetworkRequest req(url);
    req.setRawHeader("Authorization", UserManager::instance().token().toUtf8());
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject body;
    body["id"] = playlistId;
    body["name"] = name;
    if (!description.isNull()) body["description"] = description;
    auto *reply = m_nam.post(req, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            if (cb) cb(false, reply->errorString(), {});
            return;
        }
        auto doc = QJsonDocument::fromJson(reply->readAll());
        bool ok = doc.object().value("success").toBool();
        QString message = doc.object().value("message").toString();
        QVariantMap playlist;
        if (ok) {
            playlist = doc.object().value("playlist").toObject().toVariantMap();
        }
        if (cb) cb(ok, message, playlist);
    });
}

void ApiClient::deletePlaylist(int playlistId, DeletePlaylistCb cb) {
    QUrl url(QString::fromUtf8("%1/api/user/playlist/delete").arg(Theme::kApiBase));
    QNetworkRequest req(url);
    req.setRawHeader("Authorization", UserManager::instance().token().toUtf8());
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject body;
    body["id"] = playlistId;
    auto *reply = m_nam.post(req, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            if (cb) cb(false, reply->errorString());
            return;
        }
        auto doc = QJsonDocument::fromJson(reply->readAll());
        bool ok = doc.object().value("success").toBool();
        QString message = doc.object().value("message").toString();
        if (cb) cb(ok, message);
    });
}

void ApiClient::fetchPlaylistMusic(int playlistId, PlaylistMusicCb cb) {
    QUrl url(QString::fromUtf8("%1/api/user/playlist/music/%2").arg(Theme::kApiBase).arg(playlistId));
    QNetworkRequest req(url);
    if (UserManager::instance().isLoggedIn()) {
        req.setRawHeader("Authorization", UserManager::instance().token().toUtf8());
    }
    auto *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [reply, cb, playlistId]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "[歌单音乐] playlistId =" << playlistId << ", 协议:" << httpProtocolLabel(reply)
                     << ", 网络错误:" << reply->errorString();
            if (cb) cb(false, 0, {});
            return;
        }
        auto body = reply->readAll();
        auto doc = QJsonDocument::fromJson(body);
        bool ok = doc.object().value("success").toBool();
        QList<QVariantMap> res;
        int total = 0;
        if (ok) {
            total = doc.object().value("total").toInt();
            for (const auto &v : doc.object().value("musicList").toArray())
                res.append(v.toObject().toVariantMap());
            qDebug() << "[歌单音乐] playlistId =" << playlistId << ", 协议:" << httpProtocolLabel(reply)
                     << ", total =" << total << ", 实际获取 =" << res.size();
        } else {
            qDebug() << "[歌单音乐] playlistId =" << playlistId << ", 协议:" << httpProtocolLabel(reply)
                     << ", API返回success=false, body:" << body;
        }
        if (cb) cb(ok, total, res);
    });
}

void ApiClient::addMusicToPlaylist(int playlistId, int musicId, AddMusicToPlaylistCb cb) {
    QUrl url(QString::fromUtf8("%1/api/user/playlist/music/add").arg(Theme::kApiBase));
    QNetworkRequest req(url);
    req.setRawHeader("Authorization", UserManager::instance().token().toUtf8());
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject body;
    body["playlistId"] = playlistId;
    body["musicId"] = musicId;
    auto *reply = m_nam.post(req, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            if (cb) cb(false, reply->errorString());
            return;
        }
        auto doc = QJsonDocument::fromJson(reply->readAll());
        bool ok = doc.object().value("success").toBool();
        QString message = doc.object().value("message").toString();
        if (cb) cb(ok, message);
    });
}

void ApiClient::removeMusicFromPlaylist(int playlistId, int musicId, RemoveMusicFromPlaylistCb cb) {
    QUrl url(QString::fromUtf8("%1/api/user/playlist/music/remove").arg(Theme::kApiBase));
    QNetworkRequest req(url);
    req.setRawHeader("Authorization", UserManager::instance().token().toUtf8());
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject body;
    body["playlistId"] = playlistId;
    body["musicId"] = musicId;
    auto *reply = m_nam.post(req, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            if (cb) cb(false, reply->errorString());
            return;
        }
        auto doc = QJsonDocument::fromJson(reply->readAll());
        bool ok = doc.object().value("success").toBool();
        QString message = doc.object().value("message").toString();
        if (cb) cb(ok, message);
    });
}

void ApiClient::fetchFavoritePlaylists(UserPlaylistsCb cb) {
    QUrl url(QString::fromUtf8("%1/api/user/favorite-playlists").arg(Theme::kApiBase));
    QNetworkRequest req(url);
    req.setRawHeader("Authorization", UserManager::instance().token().toUtf8());
    auto *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            if (cb) cb(false, {});
            return;
        }
        auto doc = QJsonDocument::fromJson(reply->readAll());
        bool ok = doc.object().value("success").toBool();
        QList<QVariantMap> res;
        if (ok) for (const auto &v : doc.object().value("playlists").toArray())
            res.append(v.toObject().toVariantMap());
        if (cb) cb(ok, res);
    });
}

void ApiClient::favoritePlaylist(int playlistId, AddMusicToPlaylistCb cb) {
    QUrl url(QString::fromUtf8("%1/api/user/favorite-playlists").arg(Theme::kApiBase));
    QNetworkRequest req(url);
    req.setRawHeader("Authorization", UserManager::instance().token().toUtf8());
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject body;
    body["playlistId"] = playlistId;
    auto *reply = m_nam.post(req, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            if (cb) cb(false, reply->errorString());
            return;
        }
        auto doc = QJsonDocument::fromJson(reply->readAll());
        bool ok = doc.object().value("success").toBool();
        QString message = doc.object().value("message").toString();
        if (cb) cb(ok, message);
    });
}

void ApiClient::unfavoritePlaylist(int playlistId, RemoveMusicFromPlaylistCb cb) {
    QUrl url(QString::fromUtf8("%1/api/user/favorite-playlists/%2").arg(Theme::kApiBase).arg(playlistId));
    QNetworkRequest req(url);
    req.setRawHeader("Authorization", UserManager::instance().token().toUtf8());
    auto *reply = m_nam.deleteResource(req);
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            if (cb) cb(false, reply->errorString());
            return;
        }
        auto doc = QJsonDocument::fromJson(reply->readAll());
        bool ok = doc.object().value("success").toBool();
        QString message = doc.object().value("message").toString();
        if (cb) cb(ok, message);
    });
}

void ApiClient::fetchFavoritePlaylistMusic(int playlistId, PlaylistMusicCb cb) {
    QUrl url(QString::fromUtf8("%1/api/user/favorite-playlists/%2").arg(Theme::kApiBase).arg(playlistId));
    QNetworkRequest req(url);
    req.setRawHeader("Authorization", UserManager::instance().token().toUtf8());
    auto *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            if (cb) cb(false, 0, {});
            return;
        }
        auto doc = QJsonDocument::fromJson(reply->readAll());
        bool ok = doc.object().value("success").toBool();
        QList<QVariantMap> res;
        if (ok) for (const auto &v : doc.object().value("music").toArray())
            res.append(v.toObject().toVariantMap());
        if (cb) cb(ok, res.size(), res);
    });
}

void ApiClient::uploadMusic(const QString &musicFilePath, const QString &title,
                             const QString &artist, const QString &language, int duration,
                             int uploadUserId, const QString &album,
                             const QString &tags, const QString &coverFilePath,
                             const QString &lyricsFilePath, UploadCb cb) {
    QUrl url(QString::fromUtf8("%1/api/user/upload").arg(Theme::kApiBase));
    QNetworkRequest req(url);
    if (UserManager::instance().isLoggedIn()) {
        req.setRawHeader("Authorization", UserManager::instance().token().toUtf8());
    }
    auto *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    
    auto addTextPart = [multiPart](const QString &name, const QString &value) {
        QHttpPart textPart;
        textPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                           QString("form-data; name=\"%1\"").arg(name));
        textPart.setBody(value.toUtf8());
        multiPart->append(textPart);
    };
    
    addTextPart("title", title);
    addTextPart("artist", artist);
    addTextPart("language", language);
    addTextPart("duration", QString::number(duration));
    addTextPart("uploadUserId", QString::number(uploadUserId));
    if (!album.isEmpty()) addTextPart("album", album);
    if (!tags.isEmpty()) addTextPart("tags", tags);
    
    QFile *musicFile = new QFile(musicFilePath);
    if (musicFile->open(QIODevice::ReadOnly)) {
        QHttpPart musicPart;
        musicPart.setHeader(QNetworkRequest::ContentTypeHeader, "audio/mpeg");
        musicPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                            QString("form-data; name=\"musicFile\"; filename=\"%1\"")
                                .arg(musicFilePath.mid(musicFilePath.lastIndexOf('/') + 1)));
        musicPart.setBodyDevice(musicFile);
        musicFile->setParent(multiPart);
        multiPart->append(musicPart);
    } else {
        delete musicFile;
        if (cb) cb(false, "无法打开音乐文件");
        multiPart->deleteLater();
        return;
    }
    
    auto *reply = m_nam.post(req, multiPart);
    multiPart->setParent(reply);
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            if (cb) cb(false, reply->errorString());
            return;
        }
        auto doc = QJsonDocument::fromJson(reply->readAll());
        bool ok = doc.object().value("success").toBool();
        QString message = doc.object().value("message").toString();
        if (cb) cb(ok, message);
    });
}

void ApiClient::fetchVipPricing(VipPricingCb cb)
{
    QUrl url(QString::fromUtf8("%1/api/vip/pricing").arg(QString::fromUtf8(Theme::kApiBase)));
    auto *reply = m_nam.get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            if (cb)
                cb(false, reply->errorString(), {});
            return;
        }
        const auto doc = QJsonDocument::fromJson(reply->readAll());
        const auto root = doc.object();
        const bool ok = root.value(QStringLiteral("success")).toBool();
        const QString message = root.value(QStringLiteral("message")).toString();
        QList<QVariantMap> items;
        if (ok) {
            for (const auto &v : root.value(QStringLiteral("data")).toArray())
                items.append(v.toObject().toVariantMap());
        }
        if (cb)
            cb(ok, message, items);
    });
}

void ApiClient::createVipPayOrder(int pricingId, const QString &payType, VipPayCreateCb cb)
{
    if (!UserManager::instance().isLoggedIn()) {
        if (cb)
            cb(false, QString(), {});
        return;
    }
    QUrl url(QString::fromUtf8("%1/api/vip/pay/create").arg(QString::fromUtf8(Theme::kApiBase)));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setRawHeader("Authorization", UserManager::instance().token().toUtf8());
    QJsonObject body;
    body.insert(QStringLiteral("pricingId"), pricingId);
    body.insert(QStringLiteral("payType"), payType);
    auto *reply = m_nam.post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            if (cb)
                cb(false, reply->errorString(), {});
            return;
        }
        const auto doc = QJsonDocument::fromJson(reply->readAll());
        const auto root = doc.object();
        const bool ok = root.value(QStringLiteral("success")).toBool();
        const QString message = root.value(QStringLiteral("message")).toString();
        const QVariantMap data = root.value(QStringLiteral("data")).toObject().toVariantMap();
        if (cb)
            cb(ok, message, data);
    });
}

void ApiClient::syncSessionVipStatus(VipStatusCb cb)
{
    if (!UserManager::instance().isLoggedIn()) {
        if (cb)
            cb(false, false);
        return;
    }
    QUrl url(QString::fromUtf8("%1/api/user/playlists").arg(Theme::kApiBase));
    QNetworkRequest req(url);
    req.setRawHeader("Authorization", UserManager::instance().token().toUtf8());
    auto *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            if (cb)
                cb(false, UserManager::instance().isVip());
            return;
        }
        const auto doc = QJsonDocument::fromJson(reply->readAll());
        const auto root = doc.object();
        const bool ok = root.value(QStringLiteral("success")).toBool();
        const bool isVip = root.value(QStringLiteral("isVip")).toBool();
        const QString vipExpiresAt = root.value(QStringLiteral("vipExpiresAt")).toString();
        if (ok)
            UserManager::instance().updateVipStatus(isVip, vipExpiresAt);
        if (cb)
            cb(ok, isVip);
    });
}

void ApiClient::createVideoRenderJob(int musicId, double startSec, bool watermarked, VideoRenderCreateCb cb)
{
    if (!UserManager::instance().isLoggedIn()) {
        if (cb)
            cb(false, QString(), {});
        return;
    }
    QUrl url(QString::fromUtf8("%1/api/video/render/create").arg(Theme::kApiBase));
    QNetworkRequest req(url);
    req.setRawHeader("Authorization", UserManager::instance().token().toUtf8());
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    QJsonObject body;
    body[QStringLiteral("musicId")] = musicId;
    body[QStringLiteral("startSec")] = startSec;
    body[QStringLiteral("watermarked")] = watermarked;
    auto *reply = m_nam.post(req, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            if (cb)
                cb(false, reply->errorString(), {});
            return;
        }
        const auto doc = QJsonDocument::fromJson(reply->readAll());
        const auto root = doc.object();
        const bool ok = root.value(QStringLiteral("success")).toBool();
        const QString message = root.value(QStringLiteral("message")).toString();
        const QVariantMap data = root.value(QStringLiteral("data")).toObject().toVariantMap();
        if (cb)
            cb(ok, message, data);
    });
}

void ApiClient::fetchVideoRenderStatus(const QString &jobId, VideoRenderStatusCb cb)
{
    if (!UserManager::instance().isLoggedIn() || jobId.isEmpty()) {
        if (cb)
            cb(false, {});
        return;
    }
    QUrl url(QString::fromUtf8("%1/api/video/render/%2").arg(Theme::kApiBase, jobId));
    QNetworkRequest req(url);
    req.setRawHeader("Authorization", UserManager::instance().token().toUtf8());
    auto *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            if (cb)
                cb(false, {});
            return;
        }
        const auto doc = QJsonDocument::fromJson(reply->readAll());
        const auto root = doc.object();
        const bool ok = root.value(QStringLiteral("success")).toBool();
        const QVariantMap data = root.value(QStringLiteral("data")).toObject().toVariantMap();
        if (cb)
            cb(ok, data);
    });
}

void ApiClient::downloadVideoRenderFile(const QString &jobId, const QString &saveFilePath, VideoRenderDownloadCb cb)
{
    if (jobId.isEmpty() || saveFilePath.isEmpty()) {
        if (cb)
            cb(false, QStringLiteral("invalid path"));
        return;
    }
    QUrl url(QString::fromUtf8("%1/api/video/render/%2/download").arg(Theme::kApiBase, jobId));
    QNetworkRequest req(url);
    auto *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [reply, saveFilePath, cb]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            if (cb)
                cb(false, reply->errorString());
            return;
        }
        const QByteArray data = reply->readAll();
        QSaveFile out(saveFilePath);
        if (!out.open(QIODevice::WriteOnly)) {
            if (cb)
                cb(false, out.errorString());
            return;
        }
        if (out.write(data) != data.size() || !out.commit()) {
            if (cb)
                cb(false, out.errorString());
            return;
        }
        if (cb)
            cb(true, QString());
    });
}