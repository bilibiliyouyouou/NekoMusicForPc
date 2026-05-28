/**
 * @file homepage.cpp
 * @brief 首页实现 — 单一推荐歌单板块
 *
 * 推荐歌单：POST /api/playlists/search
 * 热门音乐：GET /api/music/ranking
 * 最新音乐：GET /api/music/latest
 */

#include "homepage.h"
#include "core/i18n.h"
#include "core/covercache.h"
#include "core/usermanager.h"
#include "theme/theme.h"
#include "theme/thememanager.h"
#include "ui/playlistcard.h"
#include "ui/glasswidget.h"
#include "ui/glasspaint.h"
#include "ui/scrollareafix.h"
#include "ui/svgicon.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QLabel>
#include <QPushButton>
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QtConcurrent>
#include <QDate>
#include <QPainterPath>

namespace {

void applyAggregateGlass(GlassWidget *glass)
{
    GlassPaint::applyFlatSurface(glass, Theme::ThemeManager::instance().isDarkMode());
}

} // namespace

// ─── 单曲封面标签（圆角 6px + 异步加载）─────────────────
class CoverLabel : public QLabel
{
public:
    explicit CoverLabel(int size, QWidget *parent = nullptr) : QLabel(parent), m_size(size)
    {
        setFixedSize(size, size);
        setScaledContents(false);
        setPlaceholder();
    }

    void setPlaceholder()
    {
        const bool dark = Theme::ThemeManager::instance().isDarkMode();
        m_pixmap = QPixmap(m_size, m_size);
        m_pixmap.fill(Qt::transparent);
        QPainter p(&m_pixmap);
        QPainterPath pp;
        pp.addRoundedRect(0, 0, m_size, m_size, 6, 6);
        p.fillPath(pp, dark ? QColor(128, 128, 128, 40) : QColor(111, 66, 193, 38));
        p.setClipPath(pp);
        const QColor iconColor = dark ? QColor(255, 255, 255, 100) : QColor(111, 66, 193, 140);
        auto iconPx = Icons::renderNamed("Music", 28, iconColor);
        p.drawPixmap((m_size - 28) / 2, (m_size - 28) / 2, iconPx);
        update();
    }

    void loadCover(const QString &url)
    {
        if (url.isEmpty()) { setPlaceholder(); return; }
        QString musicId = url.mid(url.lastIndexOf(QLatin1Char('/')) + 1);

        QPixmap cached = CoverCache::instance()->get(musicId);
        if (!cached.isNull()) {
            disconnect(m_coverConn);
            m_coverConn = {};
            applyPixmap(cached);
            return;
        }

        disconnect(m_coverConn);
        m_coverConn = connect(CoverCache::instance(), &CoverCache::coverLoaded, this,
                [this, musicId](const QString &id, const QPixmap &pix) {
            if (id == musicId) applyPixmap(pix);
        });
        CoverCache::instance()->fetchCover(musicId, url);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QPainterPath pp;
        pp.addRoundedRect(0, 0, m_size, m_size, 6, 6);
        p.setClipPath(pp);
        p.drawPixmap(0, 0, m_pixmap);
    }

private:
    void applyPixmap(const QPixmap &pix)
    {
        disconnect(m_coverConn);
        m_coverConn = {};
        int s = qMin(pix.width(), pix.height());
        m_pixmap = pix.copy((pix.width()-s)/2, (pix.height()-s)/2, s, s)
            .scaled(m_size, m_size, Qt::KeepAspectRatioByExpanding, Qt::FastTransformation);
        update();
    }

    QPixmap m_pixmap;
    int m_size;
    QMetaObject::Connection m_coverConn;
};

// ─── 聚合卡片（热门/最新音乐）───────────────────────
class MusicAggregateCard : public QWidget
{
public:
    enum Type { Hot, Latest };

    MusicAggregateCard(Type type, int firstId, int totalCount, QWidget *parent = nullptr)
        : QWidget(parent), m_firstId(firstId), m_type(type), m_totalCount(totalCount)
    {
        setFixedSize(212, 266);
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_StyledBackground, false);

        m_glass = new GlassWidget(this);
        m_glass->setBorderRadius(Theme::kRXl);
        m_glass->setObjectName(type == Hot ? "hpHotMusicCard" : "hpLatestMusicCard");
        applyAggregateGlass(m_glass);

        QWidget *glassBody = m_glass->contentWidget();

        auto *vlay = new QVBoxLayout(glassBody);
        vlay->setContentsMargins(12, 12, 12, 12);
        vlay->setSpacing(8);

        // 2x2 封面马赛克
        auto *grid = new QGridLayout();
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setSpacing(3);
        int coverSize = 88;
        for (int i = 0; i < 4; ++i) {
            auto *lbl = new CoverLabel(coverSize, this);
            lbl->setPlaceholder();
            m_covers.append(lbl);
            grid->addWidget(lbl, i / 2, i % 2);
        }
        vlay->addLayout(grid);

        // 标题 & 数量
        m_titleLbl = new QLabel(
            type == Hot ? I18n::instance().tr("hot_music") : I18n::instance().tr("latest_music"), glassBody);
        m_titleLbl->setObjectName(type == Hot ? "hpHotTitle" : "hpLatestTitle");
        vlay->addWidget(m_titleLbl);

        m_countLbl = new QLabel(
            QString::number(totalCount) + I18n::instance().tr("songs"), glassBody);
        m_countLbl->setObjectName(type == Hot ? "hpHotCount" : "hpLatestCount");
        vlay->addWidget(m_countLbl);

        vlay->addStretch();

        auto *outer = new QVBoxLayout(this);
        outer->setContentsMargins(0, 0, 0, 0);
        outer->addWidget(m_glass);
        applyTheme();
    }

    void applyTheme()
    {
        applyAggregateGlass(m_glass);
        for (auto *cover : m_covers)
            cover->setPlaceholder();
        const bool dark = Theme::ThemeManager::instance().isDarkMode();
        if (m_titleLbl) {
            m_titleLbl->setStyleSheet(QStringLiteral(
                "QLabel { font-size: 15px; font-weight: 700; color: %1; }")
                                          .arg(dark ? QStringLiteral("rgba(244,246,255,0.95)")
                                                    : QStringLiteral("rgba(33,37,41,0.95)")));
        }
        if (m_countLbl) {
            m_countLbl->setStyleSheet(QStringLiteral(
                "QLabel { font-size: 12px; font-weight: 500; color: %1; }")
                                          .arg(dark ? QStringLiteral("rgba(244,246,255,0.62)")
                                                    : QStringLiteral("rgba(33,37,41,0.56)")));
        }
    }

    void retranslate() {
        auto *titleLbl = findChild<QLabel *>(m_type == Hot ? "hpHotTitle" : "hpLatestTitle");
        if (titleLbl) titleLbl->setText(m_type == Hot ? I18n::instance().tr("hot_music") : I18n::instance().tr("latest_music"));
        auto *countLbl = findChild<QLabel *>(m_type == Hot ? "hpHotCount" : "hpLatestCount");
        if (countLbl) countLbl->setText(QString::number(m_totalCount) + I18n::instance().tr("songs"));
    }

    void setCover(int index, const QString &url)
    {
        if (index >= 0 && index < m_covers.size())
            m_covers[index]->loadCover(url);
    }

    int firstMusicId() const { return m_firstId; }

protected:
    void mousePressEvent(QMouseEvent *e) override
    {
        if (e->button() == Qt::LeftButton) emitClicked();
        QWidget::mousePressEvent(e);
    }

private:
    void emitClicked()
    {
        if (auto *hp = findParentHomePage()) {
            // 导航到完整列表页面而不是直接播放
            emit hp->navigateToMusicList(m_type == Hot);
        }
    }

    HomePage *findParentHomePage() const
    {
        QObject *p = parent();
        while (p) {
            if (auto *hp = qobject_cast<HomePage *>(p)) return hp;
            p = p->parent();
        }
        return nullptr;
    }

    int m_type;
    int m_firstId;
    int m_totalCount;
    GlassWidget *m_glass = nullptr;
    QList<CoverLabel *> m_covers;
    QLabel *m_titleLbl = nullptr;
    QLabel *m_countLbl = nullptr;
};

// Store for retranslate
static QList<MusicAggregateCard *> m_aggCards;

// ─── HomePage ────────────────────────────────────────

HomePage::HomePage(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, false);
    setAutoFillBackground(false);
    setupUi();

    connect(&Theme::ThemeManager::instance(), &Theme::ThemeManager::themeChanged, this,
            [this](Theme::ThemeMode) {
                for (auto *card : m_aggCards)
                    card->applyTheme();
                applyDailyEntryStyle();
            });

    // 延迟加载数据，先显示UI
    QTimer::singleShot(100, this, &HomePage::refreshData);

    // 入场淡入
    auto *eff = new QGraphicsOpacityEffect(this);
    eff->setOpacity(0.0);
    setGraphicsEffect(eff);
    auto *anim = new QPropertyAnimation(eff, "opacity");
    anim->setDuration(600);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    connect(anim, &QPropertyAnimation::finished, this, [this]() {
        setGraphicsEffect(nullptr);
    });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void HomePage::setupUi()
{
    m_scroll = new QScrollArea(this);
    m_scroll->setWidgetResizable(true);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setObjectName("hpScroll");

    auto *container = new QWidget(m_scroll);
    container->setObjectName("hpContainer");
    auto *lay = new QVBoxLayout(container);
    lay->setContentsMargins(28, 16, 28, 32);
    lay->setSpacing(20);

    m_dailyEntry = new QPushButton(container);
    m_dailyEntry->setObjectName("hpDailyRecommendEntry");
    m_dailyEntry->setCursor(Qt::PointingHandCursor);
    m_dailyEntry->setMinimumHeight(92);
    m_dailyEntry->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_dailyEntry->setFlat(true);
    auto *dailyLay = new QHBoxLayout(m_dailyEntry);
    dailyLay->setContentsMargins(12, 10, 12, 10);
    dailyLay->setSpacing(12);

    m_dailyCover = new QLabel(m_dailyEntry);
    m_dailyCover->setFixedSize(70, 70);
    m_dailyCover->setObjectName("hpDailyCover");
    dailyLay->addWidget(m_dailyCover, 0, Qt::AlignVCenter);

    auto *dailyTextWrap = new QWidget(m_dailyEntry);
    auto *dailyTextLay = new QVBoxLayout(dailyTextWrap);
    dailyTextLay->setContentsMargins(0, 2, 0, 2);
    dailyTextLay->setSpacing(6);

    auto *dailyTopRow = new QWidget(dailyTextWrap);
    auto *dailyTopLay = new QHBoxLayout(dailyTopRow);
    dailyTopLay->setContentsMargins(0, 0, 0, 0);
    dailyTopLay->setSpacing(6);

    m_dailyIcon = new QLabel(dailyTopRow);
    m_dailyIcon->setFixedSize(16, 16);
    dailyTopLay->addWidget(m_dailyIcon, 0, Qt::AlignVCenter);

    m_dailyTitle = new QLabel(dailyTopRow);
    dailyTopLay->addWidget(m_dailyTitle, 0, Qt::AlignVCenter);
    dailyTopLay->addStretch();

    m_dailyDesc = new QLabel(dailyTextWrap);
    m_dailyDesc->setWordWrap(false);

    dailyTextLay->addWidget(dailyTopRow);
    dailyTextLay->addWidget(m_dailyDesc);
    dailyLay->addWidget(dailyTextWrap, 1, Qt::AlignVCenter);

    connect(m_dailyEntry, &QPushButton::clicked, this, [this]() {
        emit navigateToDailyRecommendations();
    });
    lay->addWidget(m_dailyEntry);
    refreshDailyEntry();
    applyDailyEntryStyle();
    setDailyCoverPlaceholder();

    // ─── 推荐歌单标题 ─────────────────────────────────
    auto *titleLabel = new QLabel(I18n::instance().tr("recommend_playlists"), container);
    titleLabel->setObjectName("hpSectionTitle");
    lay->addWidget(titleLabel);
    lay->addSpacing(4);

    // ─── 卡片横向滚动区 ───────────────────────────────
    m_cardContainer = new QWidget(container);
    m_cardContainer->setObjectName("hpCardContainer");
    m_cardLayout = new QHBoxLayout(m_cardContainer);
    m_cardLayout->setContentsMargins(0, 4, 0, 0);
    m_cardLayout->setSpacing(24);
    m_cardLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    auto *loadingLabel = new QLabel(I18n::instance().tr("loading"), m_cardContainer);
    loadingLabel->setObjectName("hpLoading");
    loadingLabel->setAlignment(Qt::AlignCenter);
    m_cardLayout->addWidget(loadingLabel);

    lay->addWidget(m_cardContainer);
    lay->addStretch();

    m_scroll->setWidget(container);
    nekoPolishScrollAreaViewport(m_scroll);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(m_scroll);
}

void HomePage::refreshData()
{
    m_hotReady = false;
    m_playlistReady = false;
    m_latestReady = false;
    m_hotMusic.clear();
    m_playlists.clear();
    m_latestMusic.clear();

    fetchHotMusic();
    fetchPlaylists();
    fetchLatestMusic();
    fetchDailyEntryCover();
}

void HomePage::retranslate()
{
    refreshDailyEntry();

    auto *tl = findChild<QLabel *>("hpSectionTitle");
    if (tl) tl->setText(I18n::instance().tr("recommend_playlists"));
    auto *ll = findChild<QLabel *>("hpLoading");
    if (ll) ll->setText(I18n::instance().tr("loading"));

    for (auto *card : m_aggCards) {
        card->retranslate();
        card->applyTheme();
    }
}

void HomePage::refreshDailyEntry()
{
    if (!m_dailyEntry)
        return;
    const QString day = QDate::currentDate().toString(QStringLiteral("dd"));
    const bool dark = Theme::ThemeManager::instance().isDarkMode();
    if (m_dailyIcon) {
        QPixmap icon = Icons::renderNamed("Calendar-Empty", 18,
                                          dark ? QColor(244, 246, 255, 220)
                                               : QColor(33, 37, 41, 210));
        if (!icon.isNull()) {
            QPainter p(&icon);
            p.setRenderHint(QPainter::TextAntialiasing, true);
            QFont f = p.font();
            f.setPixelSize(8);
            f.setBold(true);
            p.setFont(f);
            p.setPen(QColor(230, 57, 80));
            p.drawText(icon.rect().adjusted(0, 5, 0, 0), Qt::AlignHCenter | Qt::AlignTop, day);
            p.end();
        }
        m_dailyIcon->setPixmap(icon);
    }
    if (m_dailyTitle)
        m_dailyTitle->setText(I18n::instance().tr("dailyRecommend"));
    if (m_dailyDesc)
        m_dailyDesc->setText(QStringLiteral("根据你的音乐口味 · 每日更新"));

}

void HomePage::setDailyCoverPlaceholder()
{
    if (!m_dailyCover)
        return;
    const bool dark = Theme::ThemeManager::instance().isDarkMode();
    QPixmap pm(70, 70);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPainterPath path;
    path.addRoundedRect(0, 0, 70, 70, 10, 10);
    QLinearGradient g(0, 0, 70, 70);
    g.setColorAt(0.0, dark ? QColor(66, 49, 84) : QColor(255, 224, 232));
    g.setColorAt(1.0, dark ? QColor(45, 34, 64) : QColor(233, 215, 255));
    p.fillPath(path, g);
    p.setClipPath(path);
    const QPixmap musicIc = Icons::renderNamed("Music", 26,
                                               dark ? QColor(255, 255, 255, 220)
                                                    : QColor(84, 62, 112, 220));
    p.drawPixmap((70 - 26) / 2, (70 - 26) / 2, musicIc);
    p.end();
    m_dailyCover->setPixmap(pm);
}

void HomePage::setDailyCoverByMusicId(int musicId)
{
    if (!m_dailyCover || musicId <= 0) {
        setDailyCoverPlaceholder();
        return;
    }
    disconnect(m_dailyCoverConn);
    m_dailyCoverConn = {};
    const QString key = QString::number(musicId);
    if (QPixmap cached = CoverCache::instance()->get(key); !cached.isNull()) {
        m_dailyCover->setPixmap(cached.scaled(70, 70, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        return;
    }
    setDailyCoverPlaceholder();
    m_dailyCoverConn = connect(CoverCache::instance(), &CoverCache::coverLoaded, this,
                               [this, key](const QString &id, const QPixmap &pix) {
                                   if (id != key || pix.isNull() || !m_dailyCover)
                                       return;
                                   m_dailyCover->setPixmap(
                                       pix.scaled(70, 70, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
                               });
    const QString coverUrl = QString::fromUtf8("%1/api/music/cover/%2").arg(Theme::kApiBase).arg(musicId);
    CoverCache::instance()->fetchCover(key, coverUrl);
}

void HomePage::fetchDailyEntryCover()
{
    if (!UserManager::instance().isLoggedIn()) {
        setDailyCoverPlaceholder();
        return;
    }
    QUrl url(QString::fromUtf8("%1/api/user/recommendations/daily").arg(Theme::kApiBase));
    QNetworkRequest req(url);
    req.setRawHeader("Authorization", UserManager::instance().token().toUtf8());
    req.setTransferTimeout(5000);
    QNetworkReply *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            setDailyCoverPlaceholder();
            return;
        }
        const QJsonObject root = QJsonDocument::fromJson(reply->readAll()).object();
        if (!root.value("success").toBool()) {
            setDailyCoverPlaceholder();
            return;
        }
        QJsonArray arr = root.value("data").toArray();
        if (arr.isEmpty())
            arr = root.value("results").toArray();
        if (arr.isEmpty())
            arr = root.value("recommendations").toArray();
        if (arr.isEmpty()) {
            setDailyCoverPlaceholder();
            return;
        }
        QJsonObject first = arr.first().toObject();
        if (first.contains("music") && first.value("music").isObject())
            first = first.value("music").toObject();
        else if (first.contains("song") && first.value("song").isObject())
            first = first.value("song").toObject();
        else if (first.contains("track") && first.value("track").isObject())
            first = first.value("track").toObject();
        int musicId = first.value("id").toInt();
        if (musicId <= 0)
            musicId = first.value("musicId").toInt();
        setDailyCoverByMusicId(musicId);
    });
}

void HomePage::applyDailyEntryStyle()
{
    if (!m_dailyEntry)
        return;
    const bool dark = Theme::ThemeManager::instance().isDarkMode();
    if (dark) {
        m_dailyEntry->setStyleSheet(
            "QPushButton#hpDailyRecommendEntry {"
            " background: rgba(28,28,28,0.56);"
            " border: 1px solid rgba(255,255,255,0.12);"
            " border-radius: 10px;"
            " text-align: left;"
            "}"
            "QPushButton#hpDailyRecommendEntry:hover {"
            " background: rgba(40,40,40,0.74);"
            " border: 1px solid rgba(255,255,255,0.22);"
            "}"
            "QPushButton#hpDailyRecommendEntry:pressed {"
            " background: rgba(48,48,48,0.82);"
            "}");
        if (m_dailyTitle)
            m_dailyTitle->setStyleSheet("QLabel { font-size: 18px; font-weight: 700; color: rgba(245,247,255,0.98); }");
        if (m_dailyDesc)
            m_dailyDesc->setStyleSheet("QLabel { font-size: 13px; font-weight: 500; color: rgba(245,247,255,0.68); }");
    } else {
        m_dailyEntry->setStyleSheet(
            "QPushButton#hpDailyRecommendEntry {"
            " background: rgba(255,255,255,0.90);"
            " border: 1px solid rgba(0,0,0,0.08);"
            " border-radius: 10px;"
            " text-align: left;"
            "}"
            "QPushButton#hpDailyRecommendEntry:hover {"
            " background: rgba(255,255,255,0.98);"
            " border: 1px solid rgba(0,0,0,0.14);"
            "}"
            "QPushButton#hpDailyRecommendEntry:pressed {"
            " background: rgba(242,243,246,0.96);"
            "}");
        if (m_dailyTitle)
            m_dailyTitle->setStyleSheet("QLabel { font-size: 18px; font-weight: 700; color: rgba(28,28,32,0.98); }");
        if (m_dailyDesc)
            m_dailyDesc->setStyleSheet("QLabel { font-size: 13px; font-weight: 500; color: rgba(28,28,32,0.62); }");
    }
}

void HomePage::fetchHotMusic()
{
    // 与旧版 Home 一致：排行榜不带 limit，卡片上的「X 首」为接口返回的全量条数（仅前 4 条用于封面拼图）
    QUrl url(QString::fromUtf8("%1/api/music/ranking").arg(Theme::kApiBase));
    QNetworkRequest req(url);
    req.setTransferTimeout(5000);
    QNetworkReply *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            m_hotReady = true;
            rebuildRecommendSection();
            return;
        }
        QByteArray rawData = reply->readAll();

        QtConcurrent::run([rawData]() {
            auto doc = QJsonDocument::fromJson(rawData);
            if (!doc.object().value("success").toBool()) {
                return QList<MusicInfo>();
            }
            QList<MusicInfo> result;
            auto arr = doc.object().value("data").toArray();
            result.reserve(arr.size());
            for (int i = 0; i < arr.size(); ++i) {
                auto obj = arr[i].toObject();
                MusicInfo info;
                info.id = obj.value("id").toInt();
                info.title = obj.value("title").toString();
                info.artist = obj.value("artist").toString();
                info.album = obj.value("album").toString();
                info.duration = obj.value("duration").toInt();
                info.coverUrl = QString::fromUtf8("%1/api/music/cover/%2")
                                    .arg(Theme::kApiBase).arg(info.id);
                result.append(info);
            }
            return result;
        }).then(this, [this](QList<MusicInfo> musicList) {
            m_hotMusic = std::move(musicList);
            m_hotReady = true;
            rebuildRecommendSection();
        });
    });
}

void HomePage::fetchPlaylists()
{
    QNetworkRequest req(QUrl(QString::fromUtf8("%1/api/playlists/search").arg(Theme::kApiBase)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setTransferTimeout(5000);
    QNetworkReply *reply = m_nam.post(req, QByteArray("{\"query\":\"\"}"));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            m_playlistReady = true;
            rebuildRecommendSection();
            return;
        }
        QByteArray rawData = reply->readAll();

        QtConcurrent::run([rawData]() {
            auto doc = QJsonDocument::fromJson(rawData);
            if (!doc.object().value("success").toBool()) {
                return QList<PlaylistInfo>();
            }
            QList<PlaylistInfo> result;
            auto arr = doc.object().value("results").toArray();
            result.reserve(arr.size());
            for (int i = 0; i < arr.size(); ++i) {
                auto obj = arr[i].toObject();
                PlaylistInfo info;
                info.id = obj.value("id").toInt();
                info.name = obj.value("name").toString();
                info.description = obj.value("description").toString();
                info.musicCount = obj.value("musicCount").toInt();
                int firstId = obj.value("firstMusicId").toInt(0);
                if (firstId > 0) {
                    info.coverUrl = QString::fromUtf8("%1/api/music/cover/%2")
                                        .arg(Theme::kApiBase).arg(firstId);
                } else {
                    info.coverUrl = QString::fromUtf8("%1/api/music/cover/1").arg(Theme::kApiBase);
                }
                result.append(info);
            }
            return result;
        }).then(this, [this](QList<PlaylistInfo> list) {
            m_playlists = std::move(list);
            m_playlistReady = true;
            rebuildRecommendSection();
        });
    });
}

void HomePage::fetchLatestMusic()
{
    // 与旧版 HomeView 一致：拉一批用于统计条数；卡片仍只展示前 4 张封面
    QUrl url(QString::fromUtf8("%1/api/music/latest?limit=300").arg(Theme::kApiBase));
    QNetworkRequest req(url);
    req.setTransferTimeout(5000);
    QNetworkReply *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            m_latestReady = true;
            rebuildRecommendSection();
            return;
        }
        QByteArray rawData = reply->readAll();

        QtConcurrent::run([rawData]() {
            auto doc = QJsonDocument::fromJson(rawData);
            if (!doc.object().value("success").toBool()) {
                return QList<MusicInfo>();
            }
            QList<MusicInfo> result;
            auto arr = doc.object().value("data").toArray();
            result.reserve(arr.size());
            for (int i = 0; i < arr.size(); ++i) {
                auto obj = arr[i].toObject();
                MusicInfo info;
                info.id = obj.value("id").toInt();
                info.title = obj.value("title").toString();
                info.artist = obj.value("artist").toString();
                info.album = obj.value("album").toString();
                info.duration = obj.value("duration").toInt();
                info.coverUrl = QString::fromUtf8("%1/api/music/cover/%2")
                                    .arg(Theme::kApiBase).arg(info.id);
                result.append(info);
            }
            return result;
        }).then(this, [this](QList<MusicInfo> musicList) {
            m_latestMusic = std::move(musicList);
            m_latestReady = true;
            rebuildRecommendSection();
        });
    });
}

void HomePage::rebuildRecommendSection()
{
    if (!m_hotReady || !m_playlistReady || !m_latestReady) return;

    // 移除"加载中"标签
    auto *loadingLabel = findChild<QLabel *>("hpLoading");
    if (loadingLabel) {
        loadingLabel->hide();
        loadingLabel->deleteLater();
    }

    QLayoutItem *item;
    while ((item = m_cardLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    m_aggCards.clear();

    // ── 热门音乐聚合卡片 ──
    {
        int firstId = m_hotMusic.isEmpty() ? 0 : m_hotMusic.first().id;
        auto *card = new MusicAggregateCard(MusicAggregateCard::Hot, firstId, m_hotMusic.size(), m_cardContainer);
        for (int i = 0; i < qMin(m_hotMusic.size(), 4); ++i) {
            card->setCover(i, m_hotMusic[i].coverUrl);
        }
        m_aggCards.append(card);
        m_cardLayout->addWidget(card);
    }

    // ── 最新音乐聚合卡片 ──
    {
        int firstId = m_latestMusic.isEmpty() ? 0 : m_latestMusic.first().id;
        auto *card = new MusicAggregateCard(MusicAggregateCard::Latest, firstId, m_latestMusic.size(), m_cardContainer);
        for (int i = 0; i < qMin(m_latestMusic.size(), 4); ++i) {
            card->setCover(i, m_latestMusic[i].coverUrl);
        }
        m_aggCards.append(card);
        m_cardLayout->addWidget(card);
    }

    // ── 推荐歌单卡片（原有样式不动）──
    for (const auto &info : m_playlists) {
        auto *card = new PlaylistCard(info, m_cardContainer);
        connect(card, &PlaylistCard::clicked, this, &HomePage::navigateToPlaylist);

        auto *eff = new QGraphicsOpacityEffect(card);
        eff->setOpacity(0.0);
        card->setGraphicsEffect(eff);
        auto *anim = new QPropertyAnimation(eff, "opacity");
        anim->setDuration(300);
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        connect(anim, &QPropertyAnimation::finished, card, [card]() {
            card->setGraphicsEffect(nullptr);
        });
        anim->start(QAbstractAnimation::DeleteWhenStopped);

        m_cardLayout->addWidget(card);
    }
}

void HomePage::paintEvent(QPaintEvent *) { /* 透明，由父窗口渐变背景透出 */ }
