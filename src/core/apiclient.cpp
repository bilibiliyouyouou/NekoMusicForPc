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
#include <QDir>
#include <QStandardPaths>
#include <QSaveFile>
#include <QMutex>
#include <QMutexLocker>

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

void ApiClient::fetchDailyRecommendations(MusicListCb cb) {
    if (!UserManager::instance().isLoggedIn()) {
        cb(false, {});
        return;
    }
    QUrl url(QString::fromUtf8("%1/api/user/recommendations/daily")
                 .arg(QString::fromUtf8(Theme::kApiBase)));
    QNetworkRequest req(url);
    req.setRawHeader("Authorization", UserManager::instance().token().toUtf8());
    auto *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [reply, cb]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) { cb(false, {}); return; }
        const auto doc = QJsonDocument::fromJson(reply->readAll());
        const auto root = doc.object();
        bool ok = root.value("success").toBool();
        QList<QVariantMap> res;
        if (ok) {
            QJsonArray arr = root.value("data").toArray();
            if (arr.isEmpty())
                arr = root.value("results").toArray();
            if (arr.isEmpty())
                arr = root.value("recommendations").toArray();

            for (const auto &v : arr) {
                QJsonObject obj = v.toObject();
                // 兼容 { music: {...} } 结构
                if (obj.contains("music") && obj.value("music").isObject())
                    obj = obj.value("music").toObject();
                res.append(obj.toVariantMap());
            }
        }
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

// ─── 网易云歌单导入 ────────────────────────────────────

void ApiClient::fetchNeteasePlaylist(qint64 playlistId, NeteasePlaylistCb cb)
{
    // 使用网易云音乐公开 API 获取歌单详情
    // 此 API 由第三方维护，可能需要替换为其他可用的服务
    QUrl url(QString::fromUtf8("https://music.163.com/api/v6/playlist/detail?id=%1").arg(playlistId));
    QNetworkRequest req(url);
    req.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
    req.setRawHeader("Referer", "https://music.163.com/");
    
    auto *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [reply, cb, playlistId]() {
        reply->deleteLater();
        NeteasePlaylistInfo info;
        info.id = playlistId;
        
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "[网易云歌单] 请求失败:" << reply->errorString();
            if (cb) cb(false, reply->errorString(), info);
            return;
        }
        
        const QByteArray data = reply->readAll();
        const auto doc = QJsonDocument::fromJson(data);
        const auto root = doc.object();
        
        // 检查 API 响应
        int code = root.value(QStringLiteral("code")).toInt();
        if (code != 200) {
            QString msg = root.value(QStringLiteral("message")).toString();
            if (msg.isEmpty()) msg = QStringLiteral("获取歌单失败");
            qDebug() << "[网易云歌单] API 返回错误:" << code << msg;
            if (cb) cb(false, msg, info);
            return;
        }
        
        // 解析歌单信息
        const auto playlist = root.value(QStringLiteral("playlist")).toObject();
        info.name = playlist.value(QStringLiteral("name")).toString();
        info.trackCount = playlist.value(QStringLiteral("trackCount")).toInt();
        
        // 解析歌曲列表
        const auto tracks = playlist.value(QStringLiteral("tracks")).toArray();
        for (const auto &trackVal : tracks) {
            const auto track = trackVal.toObject();
            NeteaseTrack t;
            t.name = track.value(QStringLiteral("name")).toString();
            
            // 获取艺术家名称
            const auto artists = track.value(QStringLiteral("ar")).toArray();
            QStringList artistNames;
            for (const auto &artistVal : artists) {
                artistNames.append(artistVal.toObject().value(QStringLiteral("name")).toString());
            }
            t.artist = artistNames.join(QStringLiteral(", "));
            
            if (!t.name.isEmpty()) {
                info.tracks.append(t);
            }
        }
        
        qDebug() << "[网易云歌单] 获取成功, 歌单:" << info.name << ", 歌曲数:" << info.tracks.size();
        if (cb) cb(true, QString(), info);
    });
}

void ApiClient::batchSearchMusic(const QList<BatchSearchItem> &items, BatchSearchCb cb)
{
    if (items.isEmpty()) {
        BatchSearchResult result;
        result.success = true;
        result.message = QStringLiteral("没有要搜索的歌曲");
        if (cb) cb(true, result);
        return;
    }
    
    // 分段处理，每批最多 20 首
    const int CHUNK_SIZE = 20;
    
    // 创建共享状态
    struct SearchState {
        QList<int> matchedIds;
        int successCount = 0;
        int failCount = 0;
        int pendingChunks = 0;
        int totalItems = 0;
        QMutex mutex;
    };
    auto *state = new SearchState();
    state->totalItems = items.size();
    
    // 分段
    QList<QList<BatchSearchItem>> chunks;
    for (int i = 0; i < items.size(); i += CHUNK_SIZE) {
        chunks.append(items.mid(i, CHUNK_SIZE));
    }
    state->pendingChunks = chunks.size();
    
    qDebug() << "[批量搜索] 共" << items.size() << "首，分" << chunks.size() << "批请求";
    
    // 逐批请求
    for (int chunkIdx = 0; chunkIdx < chunks.size(); ++chunkIdx) {
        const auto &chunk = chunks[chunkIdx];
        
        // 构建请求体 { "items": [...] }
        QJsonArray itemsArray;
        for (const auto &item : chunk) {
            QJsonObject obj;
            obj[QStringLiteral("title")] = item.title;
            obj[QStringLiteral("artist")] = item.artist;
            itemsArray.append(obj);
        }
        QJsonObject body;
        body[QStringLiteral("items")] = itemsArray;
        
        QUrl url(QString::fromUtf8("%1/api/music/search").arg(Theme::kApiBase));
        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
        req.setTransferTimeout(600000); // 10分钟超时
        
        auto *reply = m_nam.post(req, QJsonDocument(body).toJson());
        connect(reply, &QNetworkReply::finished, this, [this, reply, state, cb, chunkIdx, chunkSize = chunk.size()]() {
            reply->deleteLater();
            QMutexLocker locker(&state->mutex);
            
            state->pendingChunks--;
            
            if (reply->error() != QNetworkReply::NoError) {
                qDebug() << "[批量搜索] 第" << chunkIdx << "批请求失败:" << reply->errorString();
                state->failCount += chunkSize;
            } else {
                const auto doc = QJsonDocument::fromJson(reply->readAll());
                const auto root = doc.object();
                bool ok = root.value(QStringLiteral("success")).toBool();
                
                if (ok) {
                    // 解析 results 数组，每个元素可能是 music 对象或 null
                    const auto results = root.value(QStringLiteral("results")).toArray();
                    for (const auto &v : results) {
                        if (v.isNull()) {
                            state->failCount++;
                        } else {
                            const auto obj = v.toObject();
                            int musicId = obj.value(QStringLiteral("id")).toInt();
                            if (musicId > 0) {
                                state->matchedIds.append(musicId);
                                state->successCount++;
                            } else {
                                state->failCount++;
                            }
                        }
                    }
                    qDebug() << "[批量搜索] 第" << chunkIdx << "批成功，匹配" << results.size() << "首";
                } else {
                    QString message = root.value(QStringLiteral("message")).toString();
                    qDebug() << "[批量搜索] 第" << chunkIdx << "批API返回失败:" << message;
                    state->failCount += chunkSize;
                }
            }
            
            // 检查是否全部完成
            if (state->pendingChunks <= 0) {
                BatchSearchResult result;
                result.success = true;
                result.matchedMusicIds = state->matchedIds;
                result.successCount = state->successCount;
                result.failCount = state->failCount;
                result.message = QStringLiteral("搜索完成");
                
                qDebug() << "[批量搜索] 全部完成，成功" << state->successCount << "，失败" << state->failCount;
                
                if (cb) cb(true, result);
                delete state;
            }
        });
    }
}

void ApiClient::batchAddMusicToPlaylist(int playlistId, const QList<int> &musicIds, BatchAddMusicCb cb)
{
    if (musicIds.isEmpty()) {
        BatchAddResult result;
        result.success = true;
        result.message = QStringLiteral("没有要添加的歌曲");
        result.addedCount = 0;
        if (cb) cb(true, result);
        return;
    }
    
    if (!UserManager::instance().isLoggedIn()) {
        BatchAddResult result;
        result.success = false;
        result.message = QStringLiteral("请先登录");
        if (cb) cb(false, result);
        return;
    }
    
    // 创建共享状态
    struct AddState {
        int addedCount = 0;
        int failCount = 0;
        int pendingCount = 0;
        QMutex mutex;
    };
    auto *state = new AddState();
    state->pendingCount = musicIds.size();
    
    // 逐个添加
    for (int musicId : musicIds) {
        addMusicToPlaylist(playlistId, musicId, [state, cb, totalCount = musicIds.size()]
                          (bool ok, const QString &message) {
            QMutexLocker locker(&state->mutex);
            
            state->pendingCount--;
            
            if (ok) {
                state->addedCount++;
            } else {
                state->failCount++;
            }
            
            // 检查是否全部完成
            if (state->pendingCount <= 0) {
                BatchAddResult result;
                result.success = true;
                result.addedCount = state->addedCount;
                result.message = QString::fromUtf8("添加完成: 成功 %1, 失败 %2")
                    .arg(state->addedCount).arg(state->failCount);
                
                if (cb) cb(true, result);
                delete state;
            }
        });
    }
}
