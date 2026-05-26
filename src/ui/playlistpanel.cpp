#include "ui/playlistpanel.h"
#include "core/playlistmanager.h"
#include "core/i18n.h"
#include "theme/theme.h"
#include "theme/thememanager.h"
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
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QGraphicsDropShadowEffect>
#include <QScrollBar>
#include <QFrame>
#include <QFontMetrics>
#include <QResizeEvent>
#include <functional>

namespace {

constexpr int kPlaylistRowHeight = 64;
constexpr int kPlaylistRowSpacing = 16;
constexpr int kPlaylistRowStride = kPlaylistRowHeight + kPlaylistRowSpacing;
constexpr int kListPad = 16;

constexpr QColor kPrimary(230, 57, 80);

} // namespace

// ─── 播放队列项（对齐 SPlayer SongPlayList .song-node）────────────────
class PlaylistItemCard : public QWidget {
public:
    explicit PlaylistItemCard(const MusicInfo &info, int index, bool isCurrent, QWidget *parent = nullptr)
        : QWidget(parent), m_info(info), m_musicId(info.id), m_index(index), m_isCurrent(isCurrent)
    {
        setObjectName(QStringLiteral("PlaylistItemCard"));
        setFixedHeight(kPlaylistRowHeight);
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_StyledBackground, false);

        const bool dark = Theme::ThemeManager::instance().isDarkMode();

        auto *lay = new QHBoxLayout(this);
        lay->setContentsMargins(8, 8, 8, 8);
        lay->setSpacing(8);

        m_dragLbl = new QLabel(this);
        m_dragLbl->setFixedSize(30, 30);
        m_dragLbl->setAlignment(Qt::AlignCenter);
        m_dragLbl->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        applyDragHandleStyle(dark);
        lay->addWidget(m_dragLbl);

        m_indexLbl = new QLabel(this);
        m_indexLbl->setFixedWidth(36);
        m_indexLbl->setAlignment(Qt::AlignCenter);
        applyIndexLabelStyle(dark);
        updateIndexDisplay();
        lay->addWidget(m_indexLbl);

        auto *infoV = new QWidget(this);
        infoV->setAttribute(Qt::WA_TranslucentBackground);
        auto *infoLay = new QVBoxLayout(infoV);
        infoLay->setContentsMargins(0, 0, 0, 0);
        infoLay->setSpacing(2);

        m_titleLbl = new QLabel(info.title, infoV);
        m_titleLbl->setMinimumWidth(0);
        m_titleLbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        applyTitleStyle(dark);
        infoLay->addWidget(m_titleLbl);

        m_artistLbl = new QLabel(info.artist, infoV);
        applyArtistStyle(dark);
        infoLay->addWidget(m_artistLbl);

        lay->addWidget(infoV, 1);

        m_removeBtn = new QPushButton(this);
        m_removeBtn->setFixedSize(36, 36);
        m_removeBtn->setCursor(Qt::PointingHandCursor);
        m_removeBtn->setFlat(true);
        applyRemoveBtnStyle(dark);
        connect(m_removeBtn, &QPushButton::clicked, this, [this](bool) {
            if (removeRequested)
                removeRequested(m_musicId);
        });
        lay->addWidget(m_removeBtn);

        elideTexts();
    }

    std::function<void(int)> onClicked;
    std::function<void(int)> removeRequested;

    void bind(const MusicInfo &info, int index, bool isCurrent)
    {
        m_info = info;
        m_musicId = info.id;
        m_index = index;
        m_titleLbl->setText(info.title);
        m_artistLbl->setText(info.artist);
        updateCurrentState(isCurrent);
        elideTexts();
    }

    void updateCurrentState(bool isCurrent)
    {
        m_isCurrent = isCurrent;
        const bool dark = Theme::ThemeManager::instance().isDarkMode();
        applyDragHandleStyle(dark);
        applyIndexLabelStyle(dark);
        updateIndexDisplay();
        applyTitleStyle(dark);
        applyArtistStyle(dark);
        applyRemoveBtnStyle(dark);
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        QPainterPath path;
        path.addRoundedRect(rect(), 8, 8);

        if (m_isCurrent) {
            p.fillPath(path, QColor(kPrimary.red(), kPrimary.green(), kPrimary.blue(), 74));
            p.strokePath(path, QPen(kPrimary, 1.0));
        } else if (m_hover) {
            p.fillPath(path, QColor(kPrimary.red(), kPrimary.green(), kPrimary.blue(), 20));
            p.strokePath(path, QPen(kPrimary, 1.0));
        } else {
            p.fillPath(path, QColor(kPrimary.red(), kPrimary.green(), kPrimary.blue(), 20));
        }
    }

    void enterEvent(QEnterEvent *event) override
    {
        m_hover = true;
        update();
        QWidget::enterEvent(event);
    }

    void leaveEvent(QEvent *event) override
    {
        m_hover = false;
        update();
        QWidget::leaveEvent(event);
    }

    void resizeEvent(QResizeEvent *event) override
    {
        QWidget::resizeEvent(event);
        elideTexts();
    }

    void mousePressEvent(QMouseEvent *e) override
    {
        if (e->button() == Qt::LeftButton && onClicked)
            onClicked(m_musicId);
        QWidget::mousePressEvent(e);
    }

private:
    int textColumnWidth() const
    {
        return qMax(40, width() - 30 - 36 - 36 - 8 * 4);
    }

    void elideTexts()
    {
        const QFontMetrics titleFm(m_titleLbl->font());
        const QFontMetrics artistFm(m_artistLbl->font());
        const int w = textColumnWidth();
        m_titleLbl->setText(titleFm.elidedText(m_info.title, Qt::ElideRight, w));
        m_artistLbl->setText(artistFm.elidedText(m_info.artist, Qt::ElideRight, w));
    }

    void applyDragHandleStyle(bool dark)
    {
        const QColor ic = dark ? QColor(244, 246, 255, 77) : QColor(33, 37, 41, 115);
        m_dragLbl->setPixmap(Icons::renderNamed("Menu", 20, ic));
    }

    void applyIndexLabelStyle(bool dark)
    {
        const QString c = dark ? QString::fromUtf8(Theme::kTextMuted) : QStringLiteral("rgba(33,37,41,0.45)");
        m_indexLbl->setStyleSheet(QStringLiteral("QLabel { color: %1; font-size: 12px; }").arg(c));
    }

    void applyTitleStyle(bool dark)
    {
        const QString col = dark ? QString::fromUtf8(Theme::kTextMain) : QStringLiteral("#212529");
        m_titleLbl->setStyleSheet(QStringLiteral(
            "QLabel { font-size: 14px; font-weight: 500; color: %1; }").arg(col));
    }

    void applyArtistStyle(bool dark)
    {
        const QString col = dark ? QString::fromUtf8(Theme::kTextSub) : QStringLiteral("rgba(33,37,41,0.62)");
        m_artistLbl->setStyleSheet(QStringLiteral("QLabel { font-size: 12px; color: %1; }").arg(col));
    }

    void applyRemoveBtnStyle(bool dark)
    {
        const QColor ic = dark ? QColor(244, 246, 255, 140) : QColor(33, 37, 41, 140);
        m_removeBtn->setIcon(Icons::renderNamed("Delete", 20, ic));
        m_removeBtn->setIconSize(QSize(20, 20));
        m_removeBtn->setStyleSheet(QStringLiteral(
            "QPushButton { background: transparent; border: none; border-radius: 8px; }"
            "QPushButton:hover { background: rgba(255,255,255,0.08); }"));
    }

    void updateIndexDisplay()
    {
        if (m_isCurrent) {
            const QColor ac = kPrimary;
            m_indexLbl->setPixmap(Icons::renderNamed("Music", 20, ac));
            m_indexLbl->setText(QString());
        } else {
            m_indexLbl->setPixmap(QPixmap());
            m_indexLbl->setText(QString::number(m_index + 1));
        }
    }

    MusicInfo m_info;
    int m_musicId;
    int m_index;
    bool m_isCurrent = false;
    bool m_hover = false;
    QLabel *m_dragLbl = nullptr;
    QLabel *m_indexLbl = nullptr;
    QLabel *m_titleLbl = nullptr;
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
    card->bind(info, row, isCurrent);

    const int musicId = info.id;
    card->onClicked = [playCb, musicId](int) { playCb(musicId); };
    card->removeRequested = [](int localId) {
        PlaylistManager::instance().removeFromPlaylist(localId);
    };

    const int w = qMax(40, container->width() - 2 * kListPad);
    card->setParent(container);
    card->setFixedSize(w, kPlaylistRowHeight);
    card->move(kListPad, kListPad + row * kPlaylistRowStride);
    card->show();
    rowCards[row] = card;
}

// ─── PlaylistPanel ──────────────────────────────────────

PlaylistPanel::PlaylistPanel(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedWidth(kDrawerWidth);
    hide();

    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(28);
    shadow->setOffset(-6, 0);
    shadow->setColor(QColor(0, 0, 0, 100));
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
    const QString textMain = dark ? QString::fromUtf8(Theme::kTextMain) : QStringLiteral("#212529");
    const QString textSub = dark ? QString::fromUtf8(Theme::kTextSub) : QStringLiteral("rgba(33,37,41,0.62)");

    if (m_titleLabel) {
        m_titleLabel->setStyleSheet(QStringLiteral(
            "QLabel { font-size: 16px; font-weight: 700; color: %1; }").arg(textMain));
    }
    if (m_countLabel) {
        m_countLabel->setStyleSheet(QStringLiteral(
            "QLabel { font-size: 12px; color: %1; margin-top: 2px; }").arg(textSub));
    }
    const QString footerBtn = QStringLiteral(
        "QPushButton { background: rgba(255,255,255,%1); border: none; border-radius: 8px; "
        "color: %2; font-size: 14px; font-weight: 500; min-height: 40px; padding: 0 12px; }"
        "QPushButton:hover { background: rgba(255,255,255,%3); }")
                                    .arg(dark ? 8 : 12)
                                    .arg(textMain)
                                    .arg(dark ? 14 : 20);
    if (m_clearBtn)
        m_clearBtn->setStyleSheet(footerBtn);
    if (m_scrollCurrentBtn)
        m_scrollCurrentBtn->setStyleSheet(footerBtn);
    const QColor closeIc = dark ? QColor(244, 246, 255, 180) : QColor(33, 37, 41, 180);
    if (m_closeBtn) {
        m_closeBtn->setIcon(Icons::renderNamed("Close", 18, closeIc));
        m_closeBtn->setIconSize(QSize(18, 18));
        m_closeBtn->setStyleSheet(QStringLiteral(
            "QPushButton { background: transparent; border: none; border-radius: 8px; min-width: 32px; min-height: 32px; }"
            "QPushButton:hover { background: rgba(255,255,255,0.08); }"));
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
    updateFooterButtonIcons();
}

void PlaylistPanel::setupUi()
{
    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    auto *header = new QWidget(this);
    header->setObjectName(QStringLiteral("ppHeader"));
    auto *headerLay = new QHBoxLayout(header);
    headerLay->setContentsMargins(16, 16, 12, 8);

    auto *titleCol = new QVBoxLayout();
    titleCol->setSpacing(0);
    m_titleLabel = new QLabel(I18n::instance().tr(QStringLiteral("playQueue")), header);
    m_countLabel = new QLabel(header);
    titleCol->addWidget(m_titleLabel);
    titleCol->addWidget(m_countLabel);
    headerLay->addLayout(titleCol, 1);

    m_closeBtn = new QPushButton(header);
    m_closeBtn->setFixedSize(32, 32);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setFlat(true);
    connect(m_closeBtn, &QPushButton::clicked, this, &PlaylistPanel::closeDrawer);
    headerLay->addWidget(m_closeBtn, 0, Qt::AlignTop);

    lay->addWidget(header);

    m_scroll = new QScrollArea(this);
    m_scroll->setObjectName(QStringLiteral("ppScroll"));
    m_scroll->setWidgetResizable(true);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setFrameShape(QFrame::NoFrame);

    m_listContainer = new QWidget(m_scroll);

    m_emptyLabel = new QLabel(I18n::instance().tr(QStringLiteral("emptyPlaylistHint")), m_listContainer);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setWordWrap(true);
    m_emptyLabel->hide();

    m_scroll->setWidget(m_listContainer);
    nekoPolishScrollAreaViewport(m_scroll);
    lay->addWidget(m_scroll, 1);

    m_footer = new QWidget(this);
    auto *footerLay = new QHBoxLayout(m_footer);
    footerLay->setContentsMargins(16, 8, 16, 16);
    footerLay->setSpacing(16);

    m_clearBtn = new QPushButton(m_footer);
    m_clearBtn->setCursor(Qt::PointingHandCursor);
    m_clearBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(m_clearBtn, &QPushButton::clicked, this, [this]() {
        auto &manager = PlaylistManager::instance();
        const int currentIndex = manager.currentIndex();
        MusicInfo currentMusic;
        const bool hasCurrent = (currentIndex >= 0 && currentIndex < manager.count());
        if (hasCurrent)
            currentMusic = manager.playlist()[currentIndex];

        if (hasCurrent) {
            manager.clearPlaylist();
            manager.addToPlaylist(currentMusic);
            manager.setCurrentIndex(0);
        } else {
            manager.clearPlaylist();
        }
        refresh();
    });
    footerLay->addWidget(m_clearBtn, 1);

    m_scrollCurrentBtn = new QPushButton(m_footer);
    m_scrollCurrentBtn->setCursor(Qt::PointingHandCursor);
    m_scrollCurrentBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(m_scrollCurrentBtn, &QPushButton::clicked, this, &PlaylistPanel::scrollToCurrent);
    footerLay->addWidget(m_scrollCurrentBtn, 1);

    lay->addWidget(m_footer);

    connect(m_scroll->verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
        updateVisibleRows();
    });

    updateFooterButtonIcons();
}

void PlaylistPanel::updateFooterButtonIcons()
{
    const bool dark = Theme::ThemeManager::instance().isDarkMode();
    const QColor ic = dark ? QColor(244, 246, 255, 200) : QColor(33, 37, 41, 200);
    if (m_clearBtn) {
        m_clearBtn->setText(I18n::instance().tr(QStringLiteral("clear")));
        m_clearBtn->setIcon(Icons::renderNamed("DeleteSweep", 18, ic));
        m_clearBtn->setIconSize(QSize(18, 18));
    }
    if (m_scrollCurrentBtn) {
        m_scrollCurrentBtn->setText(I18n::instance().tr(QStringLiteral("scrollToCurrentPlay")));
        m_scrollCurrentBtn->setIcon(Icons::renderNamed("Location", 18, ic));
        m_scrollCurrentBtn->setIconSize(QSize(18, 18));
    }
}

void PlaylistPanel::updateCountLabel()
{
    if (m_countLabel) {
        m_countLabel->setText(
            I18n::instance().tr(QStringLiteral("playlistSongCount"))
                .arg(PlaylistManager::instance().count()));
    }
}

void PlaylistPanel::syncContainerHeight()
{
    const int count = PlaylistManager::instance().count();
    const int contentH = count > 0
        ? kListPad * 2 + count * kPlaylistRowStride - kPlaylistRowSpacing
        : 0;
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
    const int first = qMax(0, (scrollY - kListPad) / kPlaylistRowStride - kVisibleBuffer);
    const int last = qMin(count - 1, (scrollY + viewH - kListPad) / kPlaylistRowStride + kVisibleBuffer);

    QList<int> stale;
    for (auto it = m_rowCards.constBegin(); it != m_rowCards.constEnd(); ++it) {
        if (it.key() < first || it.key() > last)
            stale.append(it.key());
    }
    for (int row : stale)
        releasePlaylistCard(static_cast<PlaylistItemCard *>(m_rowCards.take(row)), m_cardPool);

    const int w = qMax(40, m_listContainer->width() - 2 * kListPad);
    const auto playCb = [this](int musicId) { emit playRequested(musicId); };
    for (int row = first; row <= last; ++row) {
        if (m_rowCards.contains(row)) {
            auto *card = static_cast<PlaylistItemCard *>(m_rowCards[row]);
            card->move(kListPad, kListPad + row * kPlaylistRowStride);
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

void PlaylistPanel::scrollToCurrent()
{
    const int cur = PlaylistManager::instance().currentIndex();
    if (cur < 0 || !m_scroll)
        return;
    const int y = kListPad + cur * kPlaylistRowStride;
    const int viewH = m_scroll->viewport()->height();
    const int itemBottom = y + kPlaylistRowHeight;
    int scrollVal = m_scroll->verticalScrollBar()->value();
    if (y < scrollVal)
        scrollVal = y;
    else if (itemBottom > scrollVal + viewH)
        scrollVal = itemBottom - viewH;
    m_scroll->verticalScrollBar()->setValue(scrollVal);
    updateVisibleRows();
}

void PlaylistPanel::retranslate()
{
    if (m_titleLabel)
        m_titleLabel->setText(I18n::instance().tr(QStringLiteral("playQueue")));
    if (m_emptyLabel)
        m_emptyLabel->setText(I18n::instance().tr(QStringLiteral("emptyPlaylistHint")));
    updateCountLabel();
    updateFooterButtonIcons();
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
    const int w = qMax(40, m_listContainer->width() - 2 * kListPad);
    for (auto *card : m_rowCards)
        static_cast<PlaylistItemCard *>(card)->setFixedWidth(w);
}

void PlaylistPanel::syncToHost()
{
    QWidget *host = parentWidget();
    if (!host)
        return;
    const int h = host->height();
    setFixedHeight(h);
    const int x = m_drawerOpen ? host->width() - kDrawerWidth : host->width();
    setGeometry(x, 0, kDrawerWidth, h);
}

void PlaylistPanel::openDrawer()
{
    QWidget *host = parentWidget();
    if (!host)
        return;

    if (m_slideAnim) {
        m_slideAnim->stop();
        m_slideAnim->deleteLater();
        m_slideAnim = nullptr;
    }

    refresh();
    scrollToCurrent();

    const int h = host->height();
    setFixedHeight(h);
    show();
    raise();

    m_drawerOpen = true;
    const QRect end(host->width() - kDrawerWidth, 0, kDrawerWidth, h);
    const QRect start(host->width(), 0, kDrawerWidth, h);

    if (geometry() == end) {
        m_animating = false;
        return;
    }

    m_animating = true;
    setGeometry(start);
    m_slideAnim = new QPropertyAnimation(this, "geometry", this);
    m_slideAnim->setDuration(Theme::kAnimNormal);
    m_slideAnim->setStartValue(start);
    m_slideAnim->setEndValue(end);
    m_slideAnim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_slideAnim, &QPropertyAnimation::finished, this, [this]() {
        m_animating = false;
        m_slideAnim = nullptr;
    });
    m_slideAnim->start(QAbstractAnimation::DeleteWhenStopped);
}

void PlaylistPanel::closeDrawer()
{
    QWidget *host = parentWidget();
    if (!host) {
        hide();
        m_drawerOpen = false;
        emit drawerClosed();
        return;
    }

    if (!m_drawerOpen && !isVisible())
        return;

    if (m_slideAnim) {
        m_slideAnim->stop();
        m_slideAnim->deleteLater();
        m_slideAnim = nullptr;
    }

    m_drawerOpen = false;
    const int h = host->height();
    const QRect start(geometry());
    const QRect end(host->width(), 0, kDrawerWidth, h);

    if (!isVisible() || start == end) {
        hide();
        syncToHost();
        emit drawerClosed();
        return;
    }

    m_animating = true;
    m_slideAnim = new QPropertyAnimation(this, "geometry", this);
    m_slideAnim->setDuration(Theme::kAnimNormal);
    m_slideAnim->setStartValue(start);
    m_slideAnim->setEndValue(end);
    m_slideAnim->setEasingCurve(QEasingCurve::InCubic);
    connect(m_slideAnim, &QPropertyAnimation::finished, this, [this]() {
        m_animating = false;
        m_slideAnim = nullptr;
        hide();
        syncToHost();
        emit drawerClosed();
    });
    m_slideAnim->start(QAbstractAnimation::DeleteWhenStopped);
}

void PlaylistPanel::showPanel()
{
    openDrawer();
}

void PlaylistPanel::hidePanel()
{
    closeDrawer();
}

void PlaylistPanel::togglePanel()
{
    if (m_drawerOpen)
        closeDrawer();
    else
        openDrawer();
}

void PlaylistPanel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const bool dark = Theme::ThemeManager::instance().isDarkMode();

  // 左侧圆角 + 左边框（贴宿主右缘的抽屉）
    QPainterPath clip;
    const QRect r = rect();
    const int rad = 12;
    clip.moveTo(r.right(), r.top());
    clip.lineTo(r.left() + rad, r.top());
    clip.arcTo(r.left(), r.top(), rad * 2, rad * 2, 90, 90);
    clip.lineTo(r.left(), r.bottom() - rad);
    clip.arcTo(r.left(), r.bottom() - rad * 2, rad * 2, rad * 2, 180, 90);
    clip.lineTo(r.right(), r.bottom());
    clip.closeSubpath();

    p.fillPath(clip, QColor(QString::fromUtf8(Theme::kBgSurface)));
    p.setPen(QPen(dark ? QColor(255, 255, 255, 22) : QColor(0, 0, 0, 30), 1.0));
    p.drawPath(clip);
}
