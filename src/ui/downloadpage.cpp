/**
 * @file downloadpage.cpp
 * @brief 下载管理 — 展示用户主动下载到 Downloads/NekoMusic 的音乐
 */

#include "downloadpage.h"
#include "songlistwidget.h"
#include "core/i18n.h"
#include "core/musicdownloadmanager.h"
#include "core/playlistdb.h"
#include "core/playlistmanager.h"
#include "theme/theme.h"
#include "theme/thememanager.h"
#include "ui/svgicon.h"
#include "ui/toast.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QShowEvent>
#include <QFile>
#include <QFileInfo>

DownloadPage::DownloadPage(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("downloadPage"));
    setupUi();

    connect(&Theme::ThemeManager::instance(), &Theme::ThemeManager::themeChanged, this,
            [this](Theme::ThemeMode) { applyPageStyle(); });

    connect(&PlaylistManager::instance(), &PlaylistManager::playlistChanged, this,
            &DownloadPage::updatePlayingHighlight);
    connect(&PlaylistManager::instance(), &PlaylistManager::currentIndexChanged, this,
            &DownloadPage::updatePlayingHighlight);

    auto &mgr = MusicDownloadManager::instance();
    connect(&mgr, &MusicDownloadManager::downloadsChanged, this, &DownloadPage::refresh);
    connect(&mgr, &MusicDownloadManager::downloadCompleted, this, &DownloadPage::refresh);
}

void DownloadPage::setupUi()
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
    titleLay->setSpacing(8);
    titleLay->setAlignment(Qt::AlignBottom);

    m_titleLbl = new QLabel(I18n::instance().tr(QStringLiteral("downloadManage")), titleRow);
    titleLay->addWidget(m_titleLbl);

    m_countLbl = new QLabel(titleRow);
    titleLay->addWidget(m_countLbl);
    titleLay->addStretch();
    headerLay->addWidget(titleRow);

    m_statusHintLbl = new QLabel(m_header);
    m_statusHintLbl->hide();
    headerLay->addWidget(m_statusHintLbl);

    auto *menuRow = new QWidget(m_header);
    menuRow->setFixedHeight(40);
    auto *menuLay = new QHBoxLayout(menuRow);
    menuLay->setContentsMargins(0, 0, 0, 0);
    menuLay->setSpacing(12);

    m_playBtn = new QPushButton(I18n::instance().tr(QStringLiteral("play")), menuRow);
    m_playBtn->setCursor(Qt::PointingHandCursor);
    m_playBtn->setFixedHeight(40);
    connect(m_playBtn, &QPushButton::clicked, this, [this]() {
        if (!m_allDownloads.isEmpty())
            emit playAllRequested(m_allDownloads);
    });
    menuLay->addWidget(m_playBtn);

    m_clearBtn = new QPushButton(I18n::instance().tr(QStringLiteral("clearDownloads")), menuRow);
    m_clearBtn->setCursor(Qt::PointingHandCursor);
    m_clearBtn->setFixedHeight(40);
    connect(m_clearBtn, &QPushButton::clicked, this, &DownloadPage::confirmClearDownloads);
    menuLay->addWidget(m_clearBtn);

    menuLay->addStretch();
    headerLay->addWidget(menuRow);
    root->addWidget(m_header);

    m_songList = new SongListWidget(this);
    m_songList->onSongActivate = [this](const MusicInfo &info) { emit playRequested(info); };
    m_songList->onSongPlayNext = [this](const MusicInfo &info) { emit playRequested(info); };
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
    m_statusLabel->hide();
    emptyLay->addWidget(m_statusLabel);
    m_emptyWrap->hide();
    root->addWidget(m_emptyWrap);

    applyPageStyle();
    loadDownloads();
}

void DownloadPage::applyPageStyle()
{
    const bool dark = Theme::ThemeManager::instance().isDarkMode();
    const QString titleFg = dark ? QString::fromUtf8(Theme::kTextMain) : QStringLiteral("#212529");
    const QString metaFg = dark ? QString::fromUtf8(Theme::kTextSub) : QStringLiteral("rgba(33,37,41,0.72)");
    const QString statusFg = dark ? QString::fromUtf8(Theme::kTextSub) : QStringLiteral("rgba(33,37,41,0.55)");

    if (m_titleLbl) {
        m_titleLbl->setStyleSheet(QStringLiteral(
            "QLabel { font-size: 30px; font-weight: 700; color: %1; }").arg(titleFg));
    }
    if (m_countLbl) {
        m_countLbl->setStyleSheet(QStringLiteral(
            "QLabel { font-size: 15px; font-weight: 400; color: %1; padding-bottom: 2px; }")
                                      .arg(metaFg));
    }
    if (m_statusHintLbl) {
        m_statusHintLbl->setStyleSheet(QStringLiteral(
            "QLabel { font-size: 13px; color: %1; }").arg(metaFg));
    }
    if (m_playBtn) {
        m_playBtn->setIcon(Icons::renderNamed("Play", 18, QColor(255, 255, 255)));
        m_playBtn->setIconSize(QSize(18, 18));
        m_playBtn->setStyleSheet(QStringLiteral(
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

    const QString secondaryBg = dark ? QStringLiteral("#2a2a2a") : QStringLiteral("#f0f0f0");
    const QString secondaryFg = dark ? QString::fromUtf8(Theme::kTextMain) : QStringLiteral("#212529");
    const QColor secondaryIc = dark ? QColor(244, 246, 255, 200) : QColor(33, 37, 41, 200);
    if (m_clearBtn) {
        m_clearBtn->setIcon(Icons::renderNamed("Delete", 18, secondaryIc));
        m_clearBtn->setIconSize(QSize(18, 18));
        m_clearBtn->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background: %1;"
            "  color: %2;"
            "  border: none;"
            "  border-radius: 20px;"
            "  font-size: 14px;"
            "  font-weight: 500;"
            "  padding: 0 20px;"
            "}"
            "QPushButton:hover { background: rgba(230,57,80,0.15); }"
            "QPushButton:disabled { color: rgba(128,128,128,0.8); }")
                                     .arg(secondaryBg, secondaryFg));
    }

    if (m_statusLabel) {
        m_statusLabel->setStyleSheet(QStringLiteral(
            "QLabel { font-size: 14px; color: %1; padding: 60px 24px; }").arg(statusFg));
    }

    if (m_songList)
        m_songList->applyTheme();
}

void DownloadPage::retranslate()
{
    if (m_titleLbl)
        m_titleLbl->setText(I18n::instance().tr(QStringLiteral("downloadManage")));
    if (m_playBtn)
        m_playBtn->setText(I18n::instance().tr(QStringLiteral("play")));
    if (m_clearBtn)
        m_clearBtn->setText(I18n::instance().tr(QStringLiteral("clearDownloads")));
    if (m_songList)
        m_songList->retranslate();
    updateHeaderMeta();
    applyPageStyle();
}

void DownloadPage::refresh()
{
    loadDownloads();
}

void DownloadPage::setPlaybackPaused(bool paused)
{
    if (m_songList)
        m_songList->setPlaybackPaused(paused);
}

void DownloadPage::setFavoritedMusicIds(const QSet<int> &ids)
{
    m_favoritedIds = ids;
    if (m_songList)
        m_songList->refreshFavoriteDisplay();
}

void DownloadPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    refresh();
    updatePlayingHighlight();
}

void DownloadPage::updateHeaderMeta()
{
    if (m_countLbl) {
        m_countLbl->setText(I18n::instance().tr(QStringLiteral("favoritePageSongCount"))
                                .arg(m_allDownloads.size()));
    }
    if (m_statusHintLbl) {
        m_statusHintLbl->setText(
            I18n::instance().tr(QStringLiteral("downloadFolderHint"))
                .arg(MusicDownloadManager::instance().downloadDir()));
        m_statusHintLbl->setVisible(!m_allDownloads.isEmpty());
    }
    const bool hasSongs = !m_allDownloads.isEmpty();
    if (m_playBtn)
        m_playBtn->setEnabled(hasSongs);
    if (m_clearBtn)
        m_clearBtn->setEnabled(hasSongs);
}

int DownloadPage::currentPlayingMusicId() const
{
    const auto &mgr = PlaylistManager::instance();
    const int idx = mgr.currentIndex();
    if (idx < 0 || idx >= mgr.playlist().size())
        return -1;
    return mgr.playlist()[idx].id;
}

void DownloadPage::updatePlayingHighlight()
{
    if (m_songList)
        m_songList->setCurrentPlayingId(currentPlayingMusicId());
}

void DownloadPage::showPageStatus(const QString &text, const char *iconName)
{
    if (m_songList)
        m_songList->hide();
    if (iconName && m_emptyIcon) {
        const bool dark = Theme::ThemeManager::instance().isDarkMode();
        const QColor ic = dark ? QColor(244, 246, 255, 120) : QColor(33, 37, 41, 120);
        m_emptyIcon->setPixmap(Icons::renderNamed(iconName, 48, ic));
        m_emptyIcon->show();
    } else if (m_emptyIcon) {
        m_emptyIcon->hide();
    }
    if (m_statusLabel) {
        m_statusLabel->setText(text);
        m_statusLabel->show();
    }
    if (m_emptyWrap)
        m_emptyWrap->show();
}

void DownloadPage::hidePageStatus()
{
    if (m_songList)
        m_songList->show();
    if (m_emptyIcon)
        m_emptyIcon->hide();
    if (m_statusLabel)
        m_statusLabel->hide();
    if (m_emptyWrap)
        m_emptyWrap->hide();
}

void DownloadPage::confirmClearDownloads()
{
    if (m_allDownloads.isEmpty())
        return;

    const auto ret = QMessageBox::question(
        this, I18n::instance().tr(QStringLiteral("clearDownloads")),
        I18n::instance().tr(QStringLiteral("clearDownloadsConfirm")),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret != QMessageBox::Yes)
        return;

    for (const MusicInfo &info : m_allDownloads) {
        if (info.localPath.isEmpty())
            continue;
        const QFileInfo fi(info.localPath);
        QFile::remove(info.localPath);
        QFile::remove(fi.absolutePath() + QLatin1Char('/')
                      + fi.completeBaseName() + QStringLiteral(".lrc"));
    }

    PlaylistDatabase::instance().clearDownloads();
    Toast::show(this, I18n::instance().tr(QStringLiteral("clearDownloadsSuccess")), Toast::Success);
    loadDownloads();
}

void DownloadPage::loadDownloads()
{
    m_allDownloads.clear();

    QList<MusicInfo> records = PlaylistDatabase::instance().getDownloads();
    for (const MusicInfo &info : records) {
        if (!info.localPath.isEmpty() && QFile::exists(info.localPath))
            m_allDownloads.append(info);
        else
            PlaylistDatabase::instance().removeDownload(info.id);
    }

    updateHeaderMeta();

    if (m_allDownloads.isEmpty()) {
        if (m_songList)
            m_songList->setSongs({});
        showPageStatus(I18n::instance().tr(QStringLiteral("emptyDownloads")), "Download");
        return;
    }

    hidePageStatus();
    if (m_songList) {
        m_songList->setSongs(m_allDownloads);
        m_songList->refreshFavoriteDisplay();
        m_songList->show();
    }
    updatePlayingHighlight();
}

void DownloadPage::paintEvent(QPaintEvent *)
{
}
