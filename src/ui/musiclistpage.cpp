/**
 * @file musiclistpage.cpp
 * @brief 热门 / 最新音乐列表页实现
 */

#include "musiclistpage.h"
#include "songlistwidget.h"
#include "core/apiclient.h"
#include "core/i18n.h"
#include "core/playlistmanager.h"
#include "theme/theme.h"
#include "theme/thememanager.h"
#include "ui/svgicon.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMenu>
#include <QTimer>
#include <QShowEvent>

MusicListPage::MusicListPage(Type type, QWidget *parent)
    : QWidget(parent)
    , m_type(type)
    , m_api(new ApiClient(this))
{
    setObjectName(m_type == Hot ? QStringLiteral("hotMusicPage") : QStringLiteral("latestMusicPage"));
    setAttribute(Qt::WA_StyledBackground, false);
    setAutoFillBackground(false);
    setupUi();

    connect(&Theme::ThemeManager::instance(), &Theme::ThemeManager::themeChanged, this,
            [this](Theme::ThemeMode) { applyPageStyle(); });

    connect(&PlaylistManager::instance(), &PlaylistManager::playlistChanged, this,
            &MusicListPage::updatePlayingHighlight);
    connect(&PlaylistManager::instance(), &PlaylistManager::currentIndexChanged, this,
            &MusicListPage::updatePlayingHighlight);
}

void MusicListPage::setupUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 12, 24, 24);
    root->setSpacing(0);

    m_header = new QWidget(this);
    auto *headerLay = new QVBoxLayout(m_header);
    headerLay->setContentsMargins(0, 0, 0, 0);
    headerLay->setSpacing(12);

    auto *titleRow = new QWidget(m_header);
    auto *titleLay = new QHBoxLayout(titleRow);
    titleLay->setContentsMargins(0, 0, 0, 0);
    titleLay->setSpacing(12);
    titleLay->setAlignment(Qt::AlignVCenter);

    m_backBtn = new QPushButton(titleRow);
    m_backBtn->setFixedSize(40, 40);
    m_backBtn->setCursor(Qt::PointingHandCursor);
    m_backBtn->setFlat(true);
    connect(m_backBtn, &QPushButton::clicked, this, &MusicListPage::backRequested);
    titleLay->addWidget(m_backBtn, 0, Qt::AlignVCenter);

    auto *textCol = new QWidget(titleRow);
    auto *textLay = new QVBoxLayout(textCol);
    textLay->setContentsMargins(0, 0, 0, 0);
    textLay->setSpacing(4);

    auto *titleLine = new QWidget(textCol);
    auto *titleLineLay = new QHBoxLayout(titleLine);
    titleLineLay->setContentsMargins(0, 0, 0, 0);
    titleLineLay->setSpacing(8);
    titleLineLay->setAlignment(Qt::AlignBottom);

    m_titleLbl = new QLabel(pageTitle(), titleLine);
    titleLineLay->addWidget(m_titleLbl);

    m_countLbl = new QLabel(titleLine);
    titleLineLay->addWidget(m_countLbl);
    titleLineLay->addStretch();

    textLay->addWidget(titleLine);

    m_descLbl = new QLabel(pageDesc(), textCol);
    m_descLbl->setWordWrap(true);
    textLay->addWidget(m_descLbl);

    titleLay->addWidget(textCol, 1);

    headerLay->addWidget(titleRow);

    auto *menuRow = new QWidget(m_header);
    menuRow->setFixedHeight(40);
    auto *menuLay = new QHBoxLayout(menuRow);
    menuLay->setContentsMargins(0, 0, 0, 0);
    menuLay->setSpacing(12);

    m_playAllBtn = new QPushButton(I18n::instance().tr("playAll"), menuRow);
    m_playAllBtn->setCursor(Qt::PointingHandCursor);
    m_playAllBtn->setFixedHeight(40);
    connect(m_playAllBtn, &QPushButton::clicked, this, [this]() {
        if (!m_musicList.isEmpty())
            emit playAllRequested(m_musicList);
    });
    menuLay->addWidget(m_playAllBtn);
    menuLay->addStretch();

    headerLay->addWidget(menuRow);
    root->addWidget(m_header);

    m_songList = new SongListWidget(this);
    m_songList->setListDisplayMode(m_type == Hot ? SongListWidget::ListDisplayMode::HotRanking
                                                 : SongListWidget::ListDisplayMode::LatestUpload);
    m_songList->onSongActivate = [this](const MusicInfo &info) { emit playMusic(info); };
    m_songList->onSongPlayNext = [this](const MusicInfo &info) { emit playMusic(info); };
    m_songList->onSongContextMenu = [this](const MusicInfo &info, const QPoint &pos) {
        showSongContextMenu(info, pos);
    };
    m_songList->onUnfavorite = [this](int id) { emit favoriteRequested(id); };
    m_songList->isFavorited = [this](int id) { return m_favoritedIds.contains(id); };
    m_songList->onTogglePlayPause = [this]() { emit playPauseRequested(); };
    root->addWidget(m_songList, 1);

    m_emptyWrap = new QWidget(this);
    auto *emptyLay = new QVBoxLayout(m_emptyWrap);
    emptyLay->setAlignment(Qt::AlignCenter);
    emptyLay->setSpacing(12);
    m_emptyIcon = new QLabel(m_emptyWrap);
    m_emptyIcon->setAlignment(Qt::AlignCenter);
    m_emptyIcon->hide();
    emptyLay->addWidget(m_emptyIcon);
    m_statusLabel = new QLabel(m_emptyWrap);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->hide();
    emptyLay->addWidget(m_statusLabel);
    m_emptyWrap->hide();
    root->addWidget(m_emptyWrap);

    applyPageStyle();
    showLoadingState();
}

QString MusicListPage::pageTitle() const
{
    return m_type == Hot ? I18n::instance().tr("hotMusic") : I18n::instance().tr("latestMusic");
}

QString MusicListPage::pageDesc() const
{
    return m_type == Hot ? I18n::instance().tr("hotMusicDesc") : I18n::instance().tr("latestMusicDesc");
}

void MusicListPage::applyPageStyle()
{
    const bool dark = Theme::ThemeManager::instance().isDarkMode();
    const QString titleFg = dark ? QString::fromUtf8(Theme::kTextMain) : QStringLiteral("#212529");
    const QString metaFg = dark ? QString::fromUtf8(Theme::kTextSub) : QStringLiteral("rgba(33,37,41,0.72)");
    const QString statusFg = dark ? QString::fromUtf8(Theme::kTextSub) : QStringLiteral("rgba(33,37,41,0.55)");

    if (m_titleLbl) {
        m_titleLbl->setStyleSheet(QStringLiteral(
            "QLabel { font-size: 30px; font-weight: 700; color: %1; }").arg(titleFg));
    }
    if (m_descLbl) {
        m_descLbl->setStyleSheet(QStringLiteral(
            "QLabel { font-size: 14px; color: %1; }").arg(metaFg));
    }
    if (m_countLbl) {
        m_countLbl->setStyleSheet(QStringLiteral(
            "QLabel { font-size: 15px; font-weight: 400; color: %1; padding-bottom: 2px; }")
                                      .arg(metaFg));
    }

    if (m_backBtn) {
        const QString backBg = dark ? QStringLiteral("#2a2a2a") : QStringLiteral("#f0f0f0");
        const QColor backIc = dark ? QColor(244, 246, 255, 210) : QColor(33, 37, 41, 210);
        m_backBtn->setIcon(Icons::renderNamed("SkipPrev", 20, backIc));
        m_backBtn->setIconSize(QSize(20, 20));
        m_backBtn->setStyleSheet(QStringLiteral(
            "QPushButton { background: %1; border: none; border-radius: 20px; }"
            "QPushButton:hover { background: rgba(230,57,80,0.15); }")
                                     .arg(backBg));
    }

    if (m_playAllBtn) {
        m_playAllBtn->setIcon(Icons::renderNamed("Play", 18, QColor(255, 255, 255)));
        m_playAllBtn->setIconSize(QSize(18, 18));
        m_playAllBtn->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background: #E63950;"
            "  color: #ffffff;"
            "  border: none;"
            "  border-radius: 20px;"
            "  font-size: 14px;"
            "  font-weight: 500;"
            "  padding: 0 20px;"
            "}"
            "QPushButton:hover { background: #ff5070; }"
            "QPushButton:disabled { background: rgba(230,57,80,0.35); color: rgba(255,255,255,0.6); }"));
    }

    if (m_statusLabel) {
        m_statusLabel->setStyleSheet(QStringLiteral(
            "QLabel { font-size: 14px; color: %1; padding: 60px 24px; }").arg(statusFg));
    }

    if (m_songList)
        m_songList->applyTheme();
}

void MusicListPage::retranslate()
{
    if (m_titleLbl)
        m_titleLbl->setText(pageTitle());
    if (m_descLbl)
        m_descLbl->setText(pageDesc());
    if (m_playAllBtn)
        m_playAllBtn->setText(I18n::instance().tr("playAll"));
    if (m_songList)
        m_songList->retranslate();
    updateHeaderMeta();
    applyPageStyle();
}

void MusicListPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    updatePlayingHighlight();
}

void MusicListPage::releaseCachedData()
{
    ++m_fetchGeneration;
    m_musicList.clear();
    m_musicList.squeeze();
    showLoadingState();
}

void MusicListPage::refresh()
{
    ++m_fetchGeneration;
    m_musicList.clear();
    showLoadingState();
    fetchData();
}

void MusicListPage::showLoadingState()
{
    if (m_songList)
        m_songList->setSongs({});
    showPageStatus(I18n::instance().tr("loading"));
    updateHeaderMeta();
}

void MusicListPage::showPageStatus(const QString &text, const char *iconName)
{
    if (m_emptyIcon) {
        if (iconName) {
            const bool dark = Theme::ThemeManager::instance().isDarkMode();
            const QColor ic = dark ? QColor(244, 246, 255, 90) : QColor(33, 37, 41, 90);
            m_emptyIcon->setPixmap(Icons::renderNamed(iconName, 64, ic));
            m_emptyIcon->show();
        } else {
            m_emptyIcon->hide();
        }
    }
    if (m_statusLabel) {
        m_statusLabel->setText(text);
        m_statusLabel->show();
    }
    if (m_emptyWrap)
        m_emptyWrap->show();
    if (m_songList)
        m_songList->hide();
}

void MusicListPage::hidePageStatus()
{
    if (m_emptyIcon)
        m_emptyIcon->hide();
    if (m_statusLabel)
        m_statusLabel->hide();
    if (m_emptyWrap)
        m_emptyWrap->hide();
}

void MusicListPage::fetchData()
{
    const int gen = m_fetchGeneration;

    auto finish = [this, gen](bool success, const QList<QVariantMap> &results) {
        QTimer::singleShot(0, this, [this, success, results, gen]() {
            if (gen != m_fetchGeneration)
                return;
            m_musicList.clear();
            if (success) {
                m_musicList.reserve(results.size());
                for (const auto &item : results) {
                    MusicInfo info;
                    info.id = item.value("id").toInt();
                    info.title = item.value("title").toString();
                    info.artist = item.value("artist").toString();
                    info.album = item.value("album").toString();
                    info.duration = item.value("duration").toInt();
                    info.coverUrl = QString::fromUtf8("%1/api/music/cover/%2")
                                        .arg(Theme::kApiBase)
                                        .arg(info.id);
                    if (m_type == Hot)
                        info.playCount = item.value(QStringLiteral("playCount")).toInt();
                    else
                        info.uploadedAtMs = item.value(QStringLiteral("createdAt")).toLongLong();
                    m_musicList.append(info);
                }
            }
            presentSongs();
        });
    };

    if (m_type == Hot) {
        m_api->fetchRanking([finish](bool success, const QList<QVariantMap> &results) {
            finish(success, results);
        });
    } else {
        m_api->fetchLatest(300, [finish](bool success, const QList<QVariantMap> &results) {
            finish(success, results);
        });
    }
}

void MusicListPage::presentSongs()
{
    updateHeaderMeta();

    if (m_musicList.isEmpty()) {
        if (m_songList)
            m_songList->setSongs({});
        showPageStatus(I18n::instance().tr("noData"), "SearchOff");
        return;
    }

    hidePageStatus();
    if (m_songList) {
        m_songList->setSongs(m_musicList);
        m_songList->refreshFavoriteDisplay();
        m_songList->show();
    }
    updatePlayingHighlight();
}

void MusicListPage::updateHeaderMeta()
{
    if (m_countLbl) {
        m_countLbl->setText(
            I18n::instance().tr(QStringLiteral("favoritePageSongCount")).arg(m_musicList.size()));
    }
    if (m_playAllBtn)
        m_playAllBtn->setEnabled(!m_musicList.isEmpty());
}

int MusicListPage::currentPlayingMusicId() const
{
    const auto &mgr = PlaylistManager::instance();
    const int idx = mgr.currentIndex();
    if (idx < 0 || idx >= mgr.playlist().size())
        return -1;
    return mgr.playlist()[idx].id;
}

void MusicListPage::updatePlayingHighlight()
{
    if (m_songList)
        m_songList->setCurrentPlayingId(currentPlayingMusicId());
}

void MusicListPage::setPlaybackPaused(bool paused)
{
    if (m_songList)
        m_songList->setPlaybackPaused(paused);
}

void MusicListPage::setFavoritedMusicIds(const QSet<int> &ids)
{
    m_favoritedIds = ids;
    if (m_songList)
        m_songList->refreshFavoriteDisplay();
}

void MusicListPage::showSongContextMenu(const MusicInfo &info, const QPoint &globalPos)
{
    const bool dark = Theme::ThemeManager::instance().isDarkMode();
    QMenu menu(this);
    menu.setStyleSheet(dark
                           ? QStringLiteral(
                                 "QMenu { background: #2a2a2a; border: 1px solid rgba(255,255,255,0.1);"
                                 " border-radius: 8px; padding: 6px 0; }"
                                 "QMenu::item { color: #eee; padding: 10px 20px; }"
                                 "QMenu::item:selected { background: rgba(230,57,80,0.2); }")
                           : QStringLiteral(
                                 "QMenu { background: #fff; border: 1px solid rgba(33,37,41,0.12);"
                                 " border-radius: 8px; padding: 6px 0; }"
                                 "QMenu::item { color: #212529; padding: 10px 20px; }"
                                 "QMenu::item:selected { background: rgba(230,57,80,0.12); }"));

    QAction *queueAct = menu.addAction(I18n::instance().tr("addToQueue"));
    QAction *plAct = menu.addAction(I18n::instance().tr("addToPlaylist"));
    QAction *picked = menu.exec(globalPos);
    if (picked == queueAct)
        emit addToQueue(info);
    else if (picked == plAct)
        emit addToPlaylist(info);
}

void MusicListPage::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
}
