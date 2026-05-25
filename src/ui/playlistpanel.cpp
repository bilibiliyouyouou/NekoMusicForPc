#include "ui/playlistpanel.h"
#include "core/playlistmanager.h"
#include "core/i18n.h"
#include "core/covercache.h"
#include "theme/theme.h"
#include "theme/thememanager.h"
#include "glasspaint.h"
#include "ui/scrollareafix.h"
#include "ui/svgicon.h"

#include <QSizePolicy>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QGraphicsDropShadowEffect>
#include <QScrollBar>
#include <QFrame>
#include <QLinearGradient>
#include <QUrl>
#include <QResizeEvent>
#include <functional>

namespace {

constexpr int kPlaylistRowHeight = 64;
constexpr int kPlaylistRowSpacing = 6;
constexpr int kPlaylistRowStride = kPlaylistRowHeight + kPlaylistRowSpacing;

/** 队列里识别本地曲：有路径，或历史数据仅有负数占位 id */
bool playlistPanelEntryIsLocal(const MusicInfo &info)
{
    return info.isLocalFile() || info.id < 0;
}

} // namespace

// ─── 播放队列项卡片 ──────────────────────────────────────
class PlaylistItemCard : public QWidget {
public:
    explicit PlaylistItemCard(const MusicInfo &info, int index, bool isCurrent, QWidget *parent = nullptr)
        : QWidget(parent), m_info(info), m_musicId(info.id), m_index(index), m_isCurrent(isCurrent)
    {
        setObjectName(QStringLiteral("PlaylistItemCard"));
        setFixedHeight(64);
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_StyledBackground, false);

        const bool dark = Theme::ThemeManager::instance().isDarkMode();

        auto *lay = new QHBoxLayout(this);
        lay->setContentsMargins(10, 8, 8, 8);
        lay->setSpacing(12);

        m_indexLbl = new QLabel(this);
        m_indexLbl->setFixedSize(26, 26);
        m_indexLbl->setAlignment(Qt::AlignCenter);
        applyIndexLabelStyle(dark);
        updateIndexDisplay();
        lay->addWidget(m_indexLbl);

        m_coverLbl = new QLabel(this);
        m_coverLbl->setFixedSize(48, 48);
        m_coverLbl->setScaledContents(false);
        loadCover();
        lay->addWidget(m_coverLbl);

        auto *infoV = new QWidget(this);
        infoV->setAttribute(Qt::WA_TranslucentBackground);
        auto *infoLay = new QVBoxLayout(infoV);
        infoLay->setContentsMargins(0, 0, 0, 0);
        infoLay->setSpacing(3);

        auto *titleRow = new QWidget(infoV);
        titleRow->setAttribute(Qt::WA_TranslucentBackground);
        auto *titleRowLay = new QHBoxLayout(titleRow);
        titleRowLay->setContentsMargins(0, 0, 0, 0);
        titleRowLay->setSpacing(6);

        if (playlistPanelEntryIsLocal(info)) {
            m_localBadge = new QLabel(I18n::instance().tr(QStringLiteral("localMusicBadge")), titleRow);
            m_localBadge->setAttribute(Qt::WA_TransparentForMouseEvents, true);
            applyLocalBadgeStyle(dark);
            titleRowLay->addWidget(m_localBadge, 0, Qt::AlignVCenter);
        }

        m_titleLbl = new QLabel(info.title, titleRow);
        applyTitleStyle(dark);
        m_titleLbl->setMinimumWidth(0);
        m_titleLbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        titleRowLay->addWidget(m_titleLbl, 1, Qt::AlignVCenter);
        infoLay->addWidget(titleRow);

        m_artistLbl = new QLabel(info.artist, infoV);
        applyArtistStyle(dark);
        infoLay->addWidget(m_artistLbl);

        lay->addWidget(infoV, 1);

        m_removeBtn = new QPushButton(QStringLiteral("×"), this);
        m_removeBtn->setFixedSize(28, 28);
        m_removeBtn->setCursor(Qt::PointingHandCursor);
        applyRemoveBtnStyle(dark);
        m_removeBtn->hide();
        connect(m_removeBtn, &QPushButton::clicked, this, [this]() {
            if (removeRequested)
                removeRequested(m_musicId);
        });
        lay->addWidget(m_removeBtn);
        m_coverCacheKey = coverCacheKey();
    }

    std::function<void(int)> onClicked;
    std::function<void(int)> removeRequested;

    bool isLocalLayout() const { return m_localBadge != nullptr; }

    void bind(const MusicInfo &info, int index, bool isCurrent)
    {
        m_info = info;
        m_musicId = info.id;
        m_index = index;
        m_titleLbl->setText(info.title);
        m_artistLbl->setText(info.artist);
        const QString key = coverCacheKey();
        if (key != m_coverCacheKey) {
            m_coverCacheKey = key;
            m_coverLbl->clear();
            loadCover();
        }
        updateCurrentState(isCurrent);
    }

    void updateCurrentState(bool isCurrent)
    {
        m_isCurrent = isCurrent;
        const bool dark = Theme::ThemeManager::instance().isDarkMode();
        applyIndexLabelStyle(dark);
        updateIndexDisplay();
        applyTitleStyle(dark);
        applyArtistStyle(dark);
        if (m_localBadge)
            applyLocalBadgeStyle(dark);
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const bool dark = Theme::ThemeManager::instance().isDarkMode();

        QPainterPath path;
        path.addRoundedRect(rect().adjusted(2, 2, -2, -2), 10, 10);

        if (dark) {
            if (m_isCurrent) {
                QLinearGradient g(rect().topLeft(), rect().bottomLeft());
                g.setColorAt(0.0, QColor(230, 57, 80, 38));
                g.setColorAt(1.0, QColor(126, 200, 200, 14));
                p.fillPath(path, g);
                p.strokePath(path, QPen(QColor(230, 57, 80, 95), 1.0));
            } else if (m_hover) {
                p.fillPath(path, QColor(230, 57, 80, 18));
                p.strokePath(path, QPen(QColor(230, 57, 80, 55), 1.0));
            } else {
                p.fillPath(path, QColor(42, 36, 58, 200));
                p.strokePath(path, QPen(QColor(230, 57, 80, 32), 1.0));
            }
        } else {
            if (m_isCurrent) {
                p.fillPath(path, QColor(230, 57, 80, 42));
                p.strokePath(path, QPen(QColor(111, 66, 193, 85), 1.0));
            } else if (m_hover) {
                p.fillPath(path, QColor(230, 57, 80, 22));
                p.strokePath(path, QPen(QColor(111, 66, 193, 45), 1.0));
            } else {
                p.fillPath(path, QColor(255, 255, 255, 220));
                p.strokePath(path, QPen(QColor(111, 66, 193, 35), 1.0));
            }
        }
    }

    void enterEvent(QEnterEvent *event) override
    {
        m_hover = true;
        if (m_removeBtn)
            m_removeBtn->show();
        update();
        QWidget::enterEvent(event);
    }

    void leaveEvent(QEvent *event) override
    {
        m_hover = false;
        if (m_removeBtn)
            m_removeBtn->hide();
        update();
        QWidget::leaveEvent(event);
    }

    void mousePressEvent(QMouseEvent *e) override
    {
        if (e->button() == Qt::LeftButton && onClicked)
            onClicked(m_musicId);
        QWidget::mousePressEvent(e);
    }

private:
    void applyIndexLabelStyle(bool dark)
    {
        const QString c = dark ? QString::fromUtf8(Theme::kTextMuted) : QStringLiteral("rgba(33,37,41,0.45)");
        m_indexLbl->setStyleSheet(QStringLiteral("QLabel { color: %1; font-size: 12px; font-weight: 600; }").arg(c));
    }

    void applyTitleStyle(bool dark)
    {
        const QString col = m_isCurrent
            ? QString::fromUtf8(Theme::kLavender)
            : (dark ? QString::fromUtf8(Theme::kTextMain) : QStringLiteral("#212529"));
        m_titleLbl->setStyleSheet(QStringLiteral(
            "QLabel { font-size: 13px; font-weight: 600; color: %1; }").arg(col));
    }

    void applyArtistStyle(bool dark)
    {
        const QString col = dark ? QString::fromUtf8(Theme::kTextSub) : QStringLiteral("rgba(33,37,41,0.62)");
        m_artistLbl->setStyleSheet(QStringLiteral("QLabel { font-size: 11px; color: %1; }").arg(col));
    }

    void applyLocalBadgeStyle(bool dark)
    {
        if (!m_localBadge)
            return;
        if (dark) {
            m_localBadge->setStyleSheet(QStringLiteral(
                "QLabel { font-size: 10px; font-weight: 800; color: #100818; padding: 2px 8px; border-radius: 7px; "
                "background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #7EE8C8, stop:0.42 #C8FFD8, stop:1 #ECC8FF); "
                "border: 1px solid rgba(255,255,255,0.78); }"));
        } else {
            m_localBadge->setStyleSheet(QStringLiteral(
                "QLabel { font-size: 10px; font-weight: 800; color: #160f22; padding: 2px 8px; border-radius: 7px; "
                "background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #42C4C4, stop:0.45 #A8F0D8, stop:1 #D0B0FF); "
                "border: 1px solid rgba(70,40,120,0.45); }"));
        }
    }

    void applyRemoveBtnStyle(bool dark)
    {
        const QString fg = dark ? QString::fromUtf8(Theme::kTextMuted) : QStringLiteral("rgba(33,37,41,0.45)");
        m_removeBtn->setStyleSheet(QStringLiteral(
            "QPushButton { background: transparent; border: none; color: %1; font-size: 18px; border-radius: 14px; }"
            "QPushButton:hover { background: rgba(255,100,100,0.28); color: #ff6464; }")
                                       .arg(fg));
    }

    void updateIndexDisplay()
    {
        if (m_isCurrent) {
            const QColor ac = Theme::ThemeManager::instance().isDarkMode() ? QColor(230, 57, 80)
                                                                            : QColor(111, 66, 193);
            auto icon = Icons::renderNamed("Play", 14, ac);
            QPixmap pix(26, 26);
            pix.fill(Qt::transparent);
            QPainter p(&pix);
            p.drawPixmap(6, 6, icon);
            m_indexLbl->setPixmap(pix);
        } else {
            m_indexLbl->clear();
            m_indexLbl->setText(QString::number(m_index + 1));
        }
    }

    QString coverCacheKey() const
    {
        if (playlistPanelEntryIsLocal(m_info))
            return QStringLiteral("local:%1").arg(m_info.localPath);
        return QString::number(m_musicId);
    }

    void loadCover()
    {
        if (playlistPanelEntryIsLocal(m_info)) {
            if (m_info.isLocalFile()) {
                const QString fu = CoverCache::resolveCoverUrl(m_info.coverUrl);
                if (fu.startsWith(QLatin1String("file:"), Qt::CaseInsensitive)) {
                    QPixmap p;
                    if (p.load(QUrl(fu).toLocalFile())) {
                        applyPixmap(p);
                        return;
                    }
                }
            }
            applyUnknownCover();
            return;
        }
        const QString musicId = QString::number(m_musicId);
        CoverCache *cc = CoverCache::instance();
        if (QPixmap cached = cc->get(musicId); !cached.isNull()) {
            applyPixmap(cached);
            return;
        }
        const QString url = QString::fromUtf8("%1/api/music/cover/%2").arg(Theme::kApiBase).arg(m_musicId);
        connect(cc, &CoverCache::coverLoaded, this, [this, musicId](const QString &id, const QPixmap &pix) {
            if (id == musicId)
                applyPixmap(pix);
        });
        cc->fetchCover(musicId, url);
    }

    void applyUnknownCover()
    {
        const bool dark = Theme::ThemeManager::instance().isDarkMode();
        QPixmap pm(48, 48);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing, true);
        QPainterPath cp;
        cp.addRoundedRect(0, 0, 48, 48, 10, 10);
        p.fillPath(cp, dark ? QColor(52, 44, 72) : QColor(236, 232, 248));
        p.setPen(dark ? QColor(230, 57, 80, 200) : QColor(111, 66, 193, 180));
        QFont f = p.font();
        f.setPixelSize(11);
        f.setWeight(QFont::DemiBold);
        p.setFont(f);
        p.drawText(pm.rect(), Qt::AlignCenter, I18n::instance().tr(QStringLiteral("unknown")));
        p.end();
        m_coverLbl->setPixmap(pm);
    }

    void applyPixmap(const QPixmap &pix)
    {
        if (pix.isNull())
            return;
        int s = qMin(pix.width(), pix.height());
        QPixmap scaled = pix.copy((pix.width() - s) / 2, (pix.height() - s) / 2, s, s)
                           .scaled(48, 48, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        QPixmap rounded(48, 48);
        rounded.fill(Qt::transparent);
        QPainter p(&rounded);
        p.setRenderHint(QPainter::Antialiasing);
        QPainterPath cp;
        cp.addRoundedRect(0, 0, 48, 48, 10, 10);
        p.setClipPath(cp);
        p.drawPixmap(0, 0, scaled);
        m_coverLbl->setPixmap(rounded);
    }

    MusicInfo m_info;
    int m_musicId;
    int m_index;
    QString m_coverCacheKey;
    bool m_isCurrent = false;
    bool m_hover = false;
    QLabel *m_indexLbl = nullptr;
    QLabel *m_coverLbl = nullptr;
    QLabel *m_titleLbl = nullptr;
    QLabel *m_localBadge = nullptr;
    QLabel *m_artistLbl = nullptr;
    QPushButton *m_removeBtn = nullptr;
};

void releasePlaylistCard(PlaylistItemCard *card, QList<QWidget *> &pool)
{
    if (!card)
        return;
    card->hide();
    card->onClicked = nullptr;
    card->removeRequested = nullptr;
    pool.append(card);
}

PlaylistItemCard *acquirePlaylistCard(QWidget *container, QList<QWidget *> &pool)
{
    if (!pool.isEmpty())
        return static_cast<PlaylistItemCard *>(pool.takeLast());
    return new PlaylistItemCard(MusicInfo{}, 0, false, container);
}

void bindPlaylistCard(PlaylistItemCard *card, int row, QWidget *container,
                      QHash<int, QWidget *> &rowCards, QList<QWidget *> &pool,
                      const std::function<void(int)> &playCb)
{
    const auto &playlist = PlaylistManager::instance().playlist();
    const MusicInfo &info = playlist[row];
    const bool isCurrent = row == PlaylistManager::instance().currentIndex();
    const bool wantLocal = playlistPanelEntryIsLocal(info);

    if (card->isLocalLayout() != wantLocal) {
        releasePlaylistCard(card, pool);
        card = new PlaylistItemCard(info, row, isCurrent, container);
    } else {
        card->bind(info, row, isCurrent);
    }

    const int musicId = info.id;
    card->onClicked = [playCb, musicId](int) { playCb(musicId); };
    card->removeRequested = [](int localId) {
        PlaylistManager::instance().removeFromPlaylist(localId);
    };

    const int w = container->width();
    card->setParent(container);
    card->setFixedSize(w, kPlaylistRowHeight);
    card->move(0, row * kPlaylistRowStride);
    card->show();
    rowCards[row] = card;
}

// ─── PlaylistPanel ──────────────────────────────────────

PlaylistPanel::PlaylistPanel(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Popup);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(400, 540);

    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(28);
    shadow->setOffset(0, 6);
    shadow->setColor(QColor(0, 0, 0, 110));
    setGraphicsEffect(shadow);

    setupUi();
    applyPanelChrome();

    connect(&Theme::ThemeManager::instance(), &Theme::ThemeManager::themeChanged, this,
            [this](Theme::ThemeMode) {
                applyPanelChrome();
                refresh();
            });

    connect(&PlaylistManager::instance(), &PlaylistManager::playlistChanged, this, &PlaylistPanel::refresh);

    m_lastPlaylistSize = PlaylistManager::instance().count();
    refresh();
}

void PlaylistPanel::applyPanelChrome()
{
    const bool dark = Theme::ThemeManager::instance().isDarkMode();

    if (m_titleLabel) {
        m_titleLabel->setStyleSheet(QStringLiteral(
            "QLabel { font-size: 17px; font-weight: 700; color: %1; letter-spacing: 0.35px; }")
                                        .arg(dark ? QString::fromUtf8(Theme::kLavender) : QStringLiteral("#6F42C1")));
    }
    if (m_countLabel) {
        m_countLabel->setStyleSheet(QStringLiteral(
            "QLabel { font-size: 12px; color: %1; padding-left: 8px; }")
                                        .arg(dark ? QString::fromUtf8(Theme::kTextMuted)
                                                  : QStringLiteral("rgba(33,37,41,0.52)")));
    }
    if (m_clearBtn) {
        m_clearBtn->setStyleSheet(QStringLiteral(
            "QPushButton { background: rgba(255,100,100,%1); border: 1px solid rgba(255,100,100,%2); "
            "border-radius: 14px; color: #ff6464; font-size: 11px; font-weight: 600; padding: 0 14px; }"
            "QPushButton:hover { background: rgba(255,100,100,%3); }")
                                      .arg(dark ? 22 : 18)
                                      .arg(dark ? 42 : 38)
                                      .arg(dark ? 40 : 34));
    }
    if (m_closeBtn) {
        m_closeBtn->setStyleSheet(QStringLiteral(
            "QPushButton { background: rgba(230,57,80,%2); border: none; border-radius: 14px; color: %1; font-size: 18px; }"
            "QPushButton:hover { background: rgba(230,57,80,%3); color: %4; }")
                                      .arg(dark ? QString::fromUtf8(Theme::kTextMain) : QStringLiteral("#212529"))
                                      .arg(dark ? 14 : 20)
                                      .arg(dark ? 32 : 40)
                                      .arg(dark ? QString::fromUtf8(Theme::kLavender) : QStringLiteral("#6F42C1")));
    }
    if (m_divider) {
        m_divider->setStyleSheet(QStringLiteral(
            "QWidget#ppDivider { background: qlineargradient(x1:0,y1:0,x2:1,y2:0, "
            "stop:0 transparent, stop:0.15 rgba(230,57,80,%1), stop:0.5 rgba(230,57,80,%2), stop:0.85 rgba(230,57,80,%1), stop:1 transparent); }")
                                     .arg(dark ? 55 : 65)
                                     .arg(dark ? 28 : 38));
    }
    if (m_scroll) {
        m_scroll->setStyleSheet(QStringLiteral(
            "QScrollArea#ppScroll { border: none; background: transparent; }"
            "QScrollBar:vertical { width: 5px; background: transparent; margin: 2px 0 4px 0; }"
            "QScrollBar::handle:vertical { background: rgba(230,57,80,%1); border-radius: 3px; min-height: 40px; }"
            "QScrollBar::handle:vertical:hover { background: rgba(230,57,80,%2); }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
            "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }")
                                    .arg(dark ? 70 : 82)
                                    .arg(dark ? 108 : 125));
    }
}

void PlaylistPanel::setupUi()
{
    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(14, 14, 14, 14);
    lay->setSpacing(10);

    auto *header = new QWidget(this);
    header->setObjectName(QStringLiteral("ppHeader"));
    auto *headerLay = new QHBoxLayout(header);
    headerLay->setContentsMargins(4, 2, 4, 6);

    m_titleLabel = new QLabel(I18n::instance().tr("playlist"), header);
    headerLay->addWidget(m_titleLabel);

    m_countLabel = new QLabel(header);
    headerLay->addWidget(m_countLabel);

    headerLay->addStretch();

    m_clearBtn = new QPushButton(I18n::instance().tr("clear"), header);
    m_clearBtn->setFixedHeight(30);
    m_clearBtn->setCursor(Qt::PointingHandCursor);
    connect(m_clearBtn, &QPushButton::clicked, this, [this]() {
        auto& manager = PlaylistManager::instance();
        int currentIndex = manager.currentIndex();
        MusicInfo currentMusic;
        bool hasCurrent = (currentIndex >= 0 && currentIndex < manager.count());
        if (hasCurrent) {
            currentMusic = manager.playlist()[currentIndex];
        }

        // 清空后保留当前音乐
        if (hasCurrent) {
            manager.clearPlaylist();
            manager.addToPlaylist(currentMusic);
            manager.setCurrentIndex(0);
        } else {
            manager.clearPlaylist();
        }
        refresh();
    });
    headerLay->addWidget(m_clearBtn);

    m_closeBtn = new QPushButton(QStringLiteral("×"), header);
    m_closeBtn->setFixedSize(30, 30);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    connect(m_closeBtn, &QPushButton::clicked, this, &PlaylistPanel::hideRequested);
    headerLay->addWidget(m_closeBtn);

    lay->addWidget(header);

    m_divider = new QWidget(this);
    m_divider->setObjectName(QStringLiteral("ppDivider"));
    m_divider->setFixedHeight(1);
    lay->addWidget(m_divider);

    m_scroll = new QScrollArea(this);
    m_scroll->setObjectName(QStringLiteral("ppScroll"));
    m_scroll->setWidgetResizable(true);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setFrameShape(QFrame::NoFrame);

    m_listContainer = new QWidget(m_scroll);
    m_listContainer->setContentsMargins(2, 6, 6, 8);

    m_emptyLabel = new QLabel(I18n::instance().tr("emptyPlaylist"), m_listContainer);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setWordWrap(true);
    m_emptyLabel->hide();

    m_scroll->setWidget(m_listContainer);
    nekoPolishScrollAreaViewport(m_scroll);
    lay->addWidget(m_scroll, 1);

    connect(m_scroll->verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
        updateVisibleRows();
    });
}

void PlaylistPanel::updateCountLabel()
{
    if (m_countLabel)
        m_countLabel->setText(QStringLiteral("· %1").arg(PlaylistManager::instance().count()));
}

void PlaylistPanel::syncContainerHeight()
{
    const int count = PlaylistManager::instance().count();
    const int contentH = count > 0 ? count * kPlaylistRowStride - kPlaylistRowSpacing : 0;
    const int viewH = m_scroll ? m_scroll->viewport()->height() : 0;
    m_listContainer->setMinimumHeight(qMax(contentH, viewH));
    if (m_scroll)
        m_listContainer->setFixedWidth(m_scroll->viewport()->width());
}

void PlaylistPanel::clearAllCards()
{
    const QList<int> rows = m_rowCards.keys();
    for (int row : rows)
        releasePlaylistCard(static_cast<PlaylistItemCard *>(m_rowCards.take(row)), m_cardPool);
}

void PlaylistPanel::updateVisibleRows()
{
    const int count = PlaylistManager::instance().count();
    if (count <= 0 || !m_scroll)
        return;

    const int scrollY = m_scroll->verticalScrollBar()->value();
    const int viewH = m_scroll->viewport()->height();
    const int first = qMax(0, scrollY / kPlaylistRowStride - kVisibleBuffer);
    const int last = qMin(count - 1, (scrollY + viewH) / kPlaylistRowStride + kVisibleBuffer);

    QList<int> stale;
    for (auto it = m_rowCards.constBegin(); it != m_rowCards.constEnd(); ++it) {
        if (it.key() < first || it.key() > last)
            stale.append(it.key());
    }
    for (int row : stale)
        releasePlaylistCard(static_cast<PlaylistItemCard *>(m_rowCards.take(row)), m_cardPool);

    const int w = m_listContainer->width();
    const auto playCb = [this](int musicId) { emit playRequested(musicId); };
    for (int row = first; row <= last; ++row) {
        if (m_rowCards.contains(row)) {
            auto *card = static_cast<PlaylistItemCard *>(m_rowCards[row]);
            card->move(0, row * kPlaylistRowStride);
            card->setFixedWidth(w);
            continue;
        }
        bindPlaylistCard(acquirePlaylistCard(m_listContainer, m_cardPool), row, m_listContainer,
                         m_rowCards, m_cardPool, playCb);
    }
}

void PlaylistPanel::refresh()
{
    int scrollPos = 0;
    if (m_scroll)
        scrollPos = m_scroll->verticalScrollBar()->value();

    const int count = PlaylistManager::instance().count();
    updateCountLabel();

    if (count == 0) {
        clearAllCards();
        m_lastPlaylistSize = 0;
        m_listContainer->setMinimumHeight(0);
        if (m_emptyLabel) {
            const bool dark = Theme::ThemeManager::instance().isDarkMode();
            const QString c = dark ? QString::fromUtf8(Theme::kTextSub) : QStringLiteral("rgba(33,37,41,0.55)");
            m_emptyLabel->setStyleSheet(QStringLiteral(
                "QLabel { color: %1; font-size: 13px; padding: 48px 24px; line-height: 1.5; }")
                                            .arg(c));
            m_emptyLabel->setGeometry(m_listContainer->rect());
            m_emptyLabel->show();
        }
        return;
    }

    if (m_emptyLabel)
        m_emptyLabel->hide();

    clearAllCards();
    m_lastPlaylistSize = count;

    syncContainerHeight();
    updateVisibleRows();

    const int cur = PlaylistManager::instance().currentIndex();
    for (auto it = m_rowCards.constBegin(); it != m_rowCards.constEnd(); ++it)
        static_cast<PlaylistItemCard *>(it.value())->updateCurrentState(it.key() == cur);

    if (m_scroll)
        m_scroll->verticalScrollBar()->setValue(scrollPos);
}

void PlaylistPanel::retranslate()
{
    if (m_titleLabel)
        m_titleLabel->setText(I18n::instance().tr("playlist"));
    if (m_clearBtn)
        m_clearBtn->setText(I18n::instance().tr("clear"));
    if (m_emptyLabel)
        m_emptyLabel->setText(I18n::instance().tr("emptyPlaylist"));
    applyPanelChrome();
}

void PlaylistPanel::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (!m_scroll || !m_listContainer)
        return;
    m_listContainer->setFixedWidth(m_scroll->viewport()->width());
    if (m_emptyLabel && m_emptyLabel->isVisible())
        m_emptyLabel->setGeometry(m_listContainer->rect());
    const int w = m_listContainer->width();
    for (auto *card : m_rowCards)
        static_cast<PlaylistItemCard *>(card)->setFixedWidth(w);
}

void PlaylistPanel::showPanel()
{
    refresh();
    const int cur = PlaylistManager::instance().currentIndex();
    if (cur >= 0 && m_scroll)
        m_scroll->verticalScrollBar()->setValue(cur * kPlaylistRowStride);
    updateVisibleRows();
    show();
    raise();
}

void PlaylistPanel::hidePanel() {
    hide();
}

void PlaylistPanel::togglePanel() {
    if (isVisible()) {
        hidePanel();
    } else {
        showPanel();
    }
}

void PlaylistPanel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const bool dark = Theme::ThemeManager::instance().isDarkMode();

    QPainterPath clip;
    clip.addRoundedRect(rect().adjusted(1, 1, -1, -1), 14, 14);
    p.setClipPath(clip);
    GlassPaint::paintMainWindowDeepBackdrop(p, rect(), dark);
    p.setClipping(false);

    p.setPen(QPen(dark ? QColor(230, 57, 80, 58) : QColor(111, 66, 193, 72), 1.0));
    p.drawPath(clip);
}
