#include "songlistwidget.h"
#include "songcardwidget.h"
#include "core/i18n.h"
#include "theme/theme.h"
#include "theme/thememanager.h"
#include "ui/scrollareafix.h"

#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollBar>
#include <QFrame>
#include <QPushButton>
#include "ui/svgicon.h"

namespace {

constexpr int kCoverGap = 62; // 50 cover + 12 spacing in header title gap

} // namespace

SongListWidget::SongListWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("SongListWidget"));
    setupUi();
}

void SongListWidget::setupUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_header = new QWidget(this);
    m_header->setObjectName(QStringLiteral("songListHeader"));
    m_header->setFixedHeight(kHeaderHeight);
    auto *hdrLay = new QHBoxLayout(m_header);
    hdrLay->setContentsMargins(12, 8, 12, 8);
    hdrLay->setSpacing(0);

    m_hdrNum = new QLabel(QStringLiteral("#"), m_header);
    m_hdrNum->setFixedWidth(40);
    m_hdrNum->setAlignment(Qt::AlignCenter);
    hdrLay->addWidget(m_hdrNum);
    hdrLay->addSpacing(12);

    auto *titleGap = new QWidget(m_header);
    titleGap->setFixedWidth(kCoverGap);
    hdrLay->addWidget(titleGap);

    m_hdrTitle = new QLabel(I18n::instance().tr(QStringLiteral("listColTitle")), m_header);
    hdrLay->addWidget(m_hdrTitle, 1);

    m_hdrAlbum = new QLabel(I18n::instance().tr(QStringLiteral("listColAlbum")), m_header);
    hdrLay->addWidget(m_hdrAlbum, 1);

    m_hdrActions = new QLabel(I18n::instance().tr(QStringLiteral("listColActions")), m_header);
    m_hdrActions->setFixedWidth(40);
    m_hdrActions->setAlignment(Qt::AlignCenter);
    hdrLay->addWidget(m_hdrActions);

    m_hdrDuration = new QLabel(I18n::instance().tr(QStringLiteral("duration")), m_header);
    m_hdrDuration->setFixedWidth(50);
    m_hdrDuration->setAlignment(Qt::AlignCenter);
    hdrLay->addWidget(m_hdrDuration);

    root->addWidget(m_header);

    m_scroll = new QScrollArea(this);
    m_scroll->setObjectName(QStringLiteral("favoritesSongScroll"));
    m_scroll->setWidgetResizable(true);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setFrameShape(QFrame::NoFrame);

    m_container = new QWidget(m_scroll);
    m_container->setObjectName(QStringLiteral("songListContainer"));
    m_scroll->setWidget(m_container);
    nekoPolishScrollAreaViewport(m_scroll);

    root->addWidget(m_scroll, 1);

    m_scrollCurrentBtn = new QPushButton(this);
    m_scrollCurrentBtn->setFixedSize(42, 42);
    m_scrollCurrentBtn->setCursor(Qt::PointingHandCursor);
    m_scrollCurrentBtn->setFlat(true);
    m_scrollCurrentBtn->hide();
    connect(m_scrollCurrentBtn, &QPushButton::clicked, this, &SongListWidget::scrollToPlaying);

    m_scrollTopBtn = new QPushButton(this);
    m_scrollTopBtn->setFixedSize(42, 42);
    m_scrollTopBtn->setCursor(Qt::PointingHandCursor);
    m_scrollTopBtn->setFlat(true);
    m_scrollTopBtn->hide();
    connect(m_scrollTopBtn, &QPushButton::clicked, this, &SongListWidget::scrollToTop);

    connect(m_scroll->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int v) {
        emit scrolled(v);
        updateVisibleRows();
        if (m_scrollTopBtn)
            m_scrollTopBtn->setVisible(v > 100);
        if (m_scrollCurrentBtn)
            m_scrollCurrentBtn->setVisible(hasCurrentPlaying());
    });

    applyTheme();
}

bool SongListWidget::hasCurrentPlaying() const
{
    if (m_currentId < 0)
        return false;
    for (const MusicInfo &s : m_songs) {
        if (s.id == m_currentId)
            return true;
    }
    return false;
}

void SongListWidget::setSongs(const QList<MusicInfo> &songs)
{
    const QList<int> rows = m_rowCards.keys();
    for (int row : rows)
        releaseCard(m_rowCards.take(row));

    m_songs = songs;
    syncContainerHeight();
    updateVisibleRows();
}

void SongListWidget::setCurrentPlayingId(int musicId)
{
    m_currentId = musicId;
    refreshPlayingState();
    if (m_scrollCurrentBtn)
        m_scrollCurrentBtn->setVisible(hasCurrentPlaying());
}

void SongListWidget::setPlaybackPaused(bool paused)
{
    m_paused = paused;
    refreshPlayingState();
}

void SongListWidget::refreshPlayingState()
{
    for (auto it = m_rowCards.constBegin(); it != m_rowCards.constEnd(); ++it) {
        SongCardWidget *card = it.value();
        const bool playing = card->info().id == m_currentId;
        card->setPlaying(playing);
        card->setPaused(playing && m_paused);
    }
}

void SongListWidget::applyTheme()
{
    const bool dark = Theme::ThemeManager::instance().isDarkMode();
    const QString hdrFg = dark ? QString::fromUtf8(Theme::kTextMuted) : QStringLiteral("rgba(33,37,41,0.55)");
    const QString hdrStyle = QStringLiteral(
        "QLabel { font-size: 13px; font-weight: 700; color: %1; opacity: 0.6; }").arg(hdrFg);

    for (QLabel *lbl : {m_hdrNum, m_hdrTitle, m_hdrAlbum, m_hdrActions, m_hdrDuration}) {
        if (lbl)
            lbl->setStyleSheet(hdrStyle);
    }

    if (m_header) {
        m_header->setStyleSheet(QStringLiteral(
            "QWidget#songListHeader { background: transparent; border: 1px solid transparent; }"));
    }

    const QString floatBtn = QStringLiteral(
        "QPushButton {"
        "  background: %1;"
        "  border: 1px solid rgba(230,57,80,0.28);"
        "  border-radius: 21px;"
        "}"
        "QPushButton:hover { background: %2; }")
                                 .arg(dark ? QStringLiteral("#2a2a2a") : QStringLiteral("#ffffff"),
                                      dark ? QStringLiteral("#333333") : QStringLiteral("#f5f5f5"));
    const QColor floatIc = dark ? QColor(244, 246, 255, 210) : QColor(33, 37, 41, 210);
    if (m_scrollCurrentBtn) {
        m_scrollCurrentBtn->setIcon(Icons::renderNamed("Location", 22, floatIc));
        m_scrollCurrentBtn->setIconSize(QSize(22, 22));
        m_scrollCurrentBtn->setStyleSheet(floatBtn);
    }
    if (m_scrollTopBtn) {
        m_scrollTopBtn->setIcon(Icons::renderNamed("Up", 22, floatIc));
        m_scrollTopBtn->setIconSize(QSize(22, 22));
        m_scrollTopBtn->setStyleSheet(floatBtn);
    }

    if (m_scroll) {
        m_scroll->setStyleSheet(QStringLiteral(
            "QScrollArea#favoritesSongScroll { border: none; background: transparent; }"
            "QScrollBar:vertical { width: 6px; background: transparent; margin: 2px 0; }"
            "QScrollBar::handle:vertical { background: rgba(230,57,80,%1); border-radius: 3px; min-height: 40px; }"
            "QScrollBar::handle:vertical:hover { background: rgba(230,57,80,%2); }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
            "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }")
                                    .arg(dark ? 70 : 82)
                                    .arg(dark ? 108 : 125));
    }

    for (SongCardWidget *card : m_rowCards)
        card->applyTheme();
    for (SongCardWidget *card : m_cardPool)
        card->applyTheme();
}

void SongListWidget::retranslate()
{
    if (m_hdrTitle)
        m_hdrTitle->setText(I18n::instance().tr(QStringLiteral("listColTitle")));
    if (m_hdrAlbum)
        m_hdrAlbum->setText(I18n::instance().tr(QStringLiteral("listColAlbum")));
    if (m_hdrActions)
        m_hdrActions->setText(I18n::instance().tr(QStringLiteral("listColActions")));
    if (m_hdrDuration)
        m_hdrDuration->setText(I18n::instance().tr(QStringLiteral("duration")));
}

void SongListWidget::scrollToTop()
{
    if (m_scroll)
        m_scroll->verticalScrollBar()->setValue(0);
}

void SongListWidget::scrollToPlaying()
{
    if (m_currentId < 0 || !m_scroll)
        return;
    int row = -1;
    for (int i = 0; i < m_songs.size(); ++i) {
        if (m_songs[i].id == m_currentId) {
            row = i;
            break;
        }
    }
    if (row < 0)
        return;
    const int y = kListPad + row * kRowHeight;
    const int viewH = m_scroll->viewport()->height();
    int scrollVal = m_scroll->verticalScrollBar()->value();
    if (y < scrollVal)
        scrollVal = y;
    else if (y + kRowHeight > scrollVal + viewH)
        scrollVal = y + kRowHeight - viewH;
    m_scroll->verticalScrollBar()->setValue(scrollVal);
    updateVisibleRows();
}

void SongListWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    syncContainerHeight();
    updateVisibleRows();

    const int right = width() - 52;
    const int bottom = height() - 100;
    if (m_scrollCurrentBtn)
        m_scrollCurrentBtn->move(right, bottom - 50);
    if (m_scrollTopBtn)
        m_scrollTopBtn->move(right, bottom);
}

void SongListWidget::syncContainerHeight()
{
    const int count = m_songs.size();
    const int contentH = count > 0 ? count * kRowHeight + 80 : 0;
    const int viewH = m_scroll ? m_scroll->viewport()->height() : 0;
    m_container->setMinimumHeight(qMax(contentH, viewH));
    if (m_scroll)
        m_container->setFixedWidth(m_scroll->viewport()->width());
}

SongCardWidget *SongListWidget::acquireCard()
{
    if (m_cardPool.isEmpty())
        return new SongCardWidget(m_container);
    return m_cardPool.takeLast();
}

void SongListWidget::releaseCard(SongCardWidget *card)
{
    if (!card)
        return;
    card->hide();
    card->onActivate = nullptr;
    card->onPlayNext = nullptr;
    card->onUnfavorite = nullptr;
    card->onTogglePlayPause = nullptr;
    m_cardPool.append(card);
}

void SongListWidget::updateVisibleRows()
{
    const int count = m_songs.size();
    if (count <= 0 || !m_scroll)
        return;

    const int scrollY = m_scroll->verticalScrollBar()->value();
    const int viewH = m_scroll->viewport()->height();
    const int first = qMax(0, scrollY / kRowHeight - kVisibleBuffer);
    const int last = qMin(count - 1, (scrollY + viewH) / kRowHeight + kVisibleBuffer);

    QList<int> stale;
    for (auto it = m_rowCards.constBegin(); it != m_rowCards.constEnd(); ++it) {
        if (it.key() < first || it.key() > last)
            stale.append(it.key());
    }
    for (int row : stale)
        releaseCard(m_rowCards.take(row));

    const int w = qMax(40, m_container->width());
    for (int row = first; row <= last; ++row) {
        SongCardWidget *card = m_rowCards.value(row, nullptr);
        if (!card) {
            card = acquireCard();
            card->bind(m_songs[row], row);
            card->onActivate = onSongActivate;
            card->onPlayNext = onSongPlayNext;
            card->onUnfavorite = onUnfavorite;
            card->onTogglePlayPause = onTogglePlayPause;
            card->setParent(m_container);
            m_rowCards[row] = card;
        }
        card->setFixedSize(w, kRowHeight);
        card->move(0, row * kRowHeight);
        card->setPlaying(m_songs[row].id == m_currentId);
        card->setPaused(m_songs[row].id == m_currentId && m_paused);
        card->show();
    }
}
