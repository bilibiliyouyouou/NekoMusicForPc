#include "playerpage.h"
#include "playerprogressslider.h"
#include "videorenderdialog.h"
#include "toast.h"
#include "svgicon.h"
#include "../core/apiclient.h"
#include "../core/playerengine.h"
#include "../core/i18n.h"
#include "../core/usermanager.h"
#include "../core/covercache.h"
#include "../core/httpprotocollabel.h"
#include "../core/embeddedlyrics.h"
#include "../theme/theme.h"
#include "../theme/thememanager.h"
#include "glasspaint.h"
#include "ui/scrollareafix.h"

#include <QPainter>
#include <QStylePainter>
#include <QStyleOptionButton>

namespace {

constexpr int kPlayerCoverSize = 300;
constexpr int kPlayerCoverRadius = 32;
constexpr int kLyricPadLeft = 40;
constexpr int kPlayerStyleRatio = 50;
constexpr int kPlayerControlH = 80;
constexpr int kPpCtrlBtn = 38;
constexpr int kPpPlayBtn = 44;
constexpr int kPpCtrlIcon = 20;
constexpr int kPpPlayIcon = 26;
constexpr int kPpProgressMaxW = 480;

const QColor kPpIconAccent = QColor(255, 143, 158, 255);
const QColor kPpPlayGlyph = QColor(18, 10, 14, 255);

inline QColor ppCtrlIdleColor()
{
    return Theme::ThemeManager::instance().isDarkMode() ? QColor(255, 255, 255, 210)
                                                        : QColor(33, 37, 41, 210);
}

QString formatPlaybackTime(qint64 ms)
{
    const qint64 sec = ms / 1000;
    return QStringLiteral("%1:%2").arg(sec / 60).arg(sec % 60, 2, 10, QChar('0'));
}

enum class PpInk : int { Prev, Next, PlayMain };

class PlayerPageInkButton final : public QPushButton {
public:
    explicit PlayerPageInkButton(QWidget *parent = nullptr)
        : QPushButton(parent)
    {
        setFlat(true);
        setAutoDefault(false);
        setDefault(false);
        setFocusPolicy(Qt::NoFocus);
        setAttribute(Qt::WA_Hover, true);
        setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        QStylePainter sp(this);
        QStyleOptionButton opt;
        initStyleOption(&opt);
        sp.drawControl(QStyle::CE_PushButton, opt);

        const auto ink = static_cast<PpInk>(property("ppInk").toInt());
        const QSize sz = iconSize().isValid() ? iconSize() : QSize(kPpCtrlIcon, kPpCtrlIcon);
        const int px = qMax(sz.width(), sz.height());
        const bool hi = isEnabled() && (underMouse() || isDown());
        const QColor cN = ppCtrlIdleColor();
        const QColor cA = kPpIconAccent;

        QPixmap pm;
        switch (ink) {
        case PpInk::Prev:
            pm = Icons::render(Icons::kPrev, px, hi ? cA : cN);
            break;
        case PpInk::Next:
            pm = Icons::render(Icons::kNext, px, hi ? cA : cN);
            break;
        case PpInk::PlayMain:
            pm = Icons::render(property("ppPlaying").toBool() ? Icons::kPause : Icons::kPlay, px, kPpPlayGlyph);
            break;
        }
        if (pm.isNull())
            return;

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QRect r = rect();
        const QPoint topLeft(r.x() + (r.width() - pm.width()) / 2, r.y() + (r.height() - pm.height()) / 2);
        painter.drawPixmap(topLeft, pm);
        Q_UNUSED(event);
    }
};

} // namespace

#include <QPainterPath>
#include <QColor>
#include <QFrame>
#include <QFontMetrics>
#include <QResizeEvent>
#include <QShowEvent>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QScrollBar>
#include <QJsonDocument>
#include <QJsonObject>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QDebug>
#include <QUrl>
#include <QFile>
#include <QFileInfo>
#include <QTimer>
#include <QDialog>
#include <QFileDialog>
#include <QStandardPaths>
#include <QRegularExpression>

PlayerPage::PlayerPage(PlayerEngine *engine, ApiClient *apiClient, QWidget *parent)
    : QWidget(parent), m_engine(engine), m_apiClient(apiClient)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setupUi();

    connect(&Theme::ThemeManager::instance(), &Theme::ThemeManager::themeChanged, this,
            [this](Theme::ThemeMode) {
                applyPlayerPageStyle();
                update();
                if (m_ppPrevBtn)
                    m_ppPrevBtn->update();
                if (m_ppPlayBtn)
                    m_ppPlayBtn->update();
                if (m_ppNextBtn)
                    m_ppNextBtn->update();
                rebuildLyricLabels();
                // 重建后控件已换，若行号未变 updateLyricHighlight 会早退，高亮样式不会套到新 QLabel
                m_currentLyricLine = -1;
                updateLyricHighlight(m_engine->position());
            });
}

PlayerPage::~PlayerPage() = default;

void PlayerPage::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    GlassPaint::paintMainWindowDeepBackdrop(p, rect(), Theme::ThemeManager::instance().isDarkMode());

    QWidget::paintEvent(event);
}

void PlayerPage::applyPlayerPageStyle()
{
    const bool dark = Theme::ThemeManager::instance().isDarkMode();

    if (dark) {
        m_clrTitle = QString::fromUtf8(Theme::kLavender);
        m_clrArtist = QString::fromUtf8(Theme::kTextSub);
        m_clrAlbum = QString::fromUtf8(Theme::kTextMuted);
        m_clrLyricDim = QString::fromUtf8(Theme::kTextMuted);
        m_clrLyricHi = QString::fromUtf8(Theme::kLavender);
        m_clrLyricHiTrans = QString::fromUtf8(Theme::kLavenderLt);
        m_clrLyricHiBg = QStringLiteral("rgba(230,57,80,24)");
    } else {
        m_clrTitle = QStringLiteral("#6F42C1");
        m_clrArtist = QStringLiteral("rgba(33,37,41,0.78)");
        m_clrAlbum = QStringLiteral("rgba(33,37,41,0.52)");
        m_clrLyricDim = QStringLiteral("rgba(33,37,41,0.52)");
        m_clrLyricHi = QStringLiteral("#6F42C1");
        m_clrLyricHiTrans = QStringLiteral("#8B6FC4");
        m_clrLyricHiBg = QStringLiteral("rgba(230,57,80,0.38)");
    }

    const QString backFg = dark ? QString::fromUtf8(Theme::kLavender) : QStringLiteral("#6F42C1");
    const QString coverBorder = dark ? QStringLiteral("rgba(230,57,80,32)")
                                     : QStringLiteral("rgba(111,66,193,0.32)");

    const int backBgA = dark ? 18 : 22;
    const int backBdA = dark ? 32 : 40;
    const int backHiA = dark ? 42 : 55;
    const int backHiBdA = dark ? 62 : 75;
    const int sepA = dark ? 40 : 48;
    const int sbA = dark ? 60 : 85;
    const int sbHiA = dark ? 100 : 120;

    setStyleSheet(QString::fromUtf8(
                      "#playerPage { background: transparent; }"

                      "#playerBackBtn { "
                      "  background: rgba(230,57,80,%5); color: %1; font-size: 20px; "
                      "  border: 1px solid rgba(230,57,80,%6); border-radius: 22px; }"
                      "#playerBackBtn:hover { "
                      "  background: rgba(230,57,80,%7); color: %1; border-color: rgba(230,57,80,%8); }"

                      "#playerLeftPanel, #playerRightPanel { background: transparent; }"

                      "#playerCoverLabel { "
                      "  background: transparent; "
                      "  border: 2px solid %2; "
                      "  border-radius: %13px; }"

                      "#playerSongTitleLabel { "
                      "  color: %3; font-size: 26px; font-weight: 700; "
                      "  background: transparent; }"

                      "#playerMetaIcon { "
                      "  color: %4; font-size: 14px; background: transparent; }"

                      "#playerArtistLabel, #playerAlbumLabel { "
                      "  color: %4; font-size: 16px; font-weight: 400; "
                      "  background: transparent; }"

                      "#playerAlbumLabel { color: %9; }"

                      "#playerVideoRenderBtn, #playerVideoDownloadBtn { "
                      "  background: rgba(230,57,80,%5); color: %1; font-size: 13px; font-weight: 600; "
                      "  border: 1px solid rgba(230,57,80,%6); border-radius: 18px; padding: 8px 18px; }"
                      "#playerVideoRenderBtn:hover, #playerVideoDownloadBtn:hover { "
                      "  background: rgba(230,57,80,%7); border-color: rgba(230,57,80,%8); }"
                      "#playerVideoRenderBtn:disabled { "
                      "  color: rgba(255,255,255,0.35); background: rgba(120,120,120,0.25); border-color: rgba(255,255,255,0.08); }"

                      "#playerVideoStatusLbl { "
                      "  color: %4; font-size: 12px; background: transparent; "
                      "  qproperty-alignment: 'AlignCenter'; }"

                      "#lyricsScroll { "
                      "  background: transparent; border: none; }"
                      "#lyricsScroll > QWidget { background: transparent; }"

                      "QScrollBar:vertical { width: 4px; background: transparent; }"
                      "QScrollBar::handle:vertical { "
                      "  background: rgba(230,57,80,%11); border-radius: 2px; min-height: 50px; }"
                      "QScrollBar::handle:vertical:hover { "
                      "  background: rgba(230,57,80,%12); }"
                      "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
                      "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }"

                      "#playerControlBar { background: transparent; }"
                      "#ppCurTime, #ppDurTime { "
                      "  color: %4; font-size: 12px; background: transparent; }")
                      .arg(backFg)
                      .arg(coverBorder)
                      .arg(m_clrTitle)
                      .arg(m_clrArtist)
                      .arg(backBgA)
                      .arg(backBdA)
                      .arg(backHiA)
                      .arg(backHiBdA)
                      .arg(m_clrAlbum)
                      .arg(sepA)
                      .arg(sbA)
                      .arg(sbHiA)
                      .arg(kPlayerCoverRadius));

    applyMetaLabelFonts();
    applyMetaTextElide();
}

void PlayerPage::setupPlayerControl()
{
    m_controlBar = new QWidget(this);
    m_controlBar->setObjectName(QStringLiteral("playerControlBar"));
    m_controlBar->setFixedHeight(kPlayerControlH);
    m_controlBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto *barLay = new QHBoxLayout(m_controlBar);
    barLay->setContentsMargins(24, 0, 24, 0);
    barLay->setSpacing(0);

    barLay->addStretch(1);

    auto *center = new QWidget(m_controlBar);
    center->setObjectName(QStringLiteral("playerControlCenter"));
    auto *centerLay = new QVBoxLayout(center);
    centerLay->setContentsMargins(0, 0, 0, 0);
    centerLay->setSpacing(4);
    centerLay->setAlignment(Qt::AlignCenter);

    auto *btnRow = new QHBoxLayout();
    btnRow->setContentsMargins(0, 0, 0, 0);
    btnRow->setSpacing(12);
    btnRow->setAlignment(Qt::AlignCenter);

    m_ppPrevBtn = new PlayerPageInkButton(center);
    m_ppPrevBtn->setObjectName(QStringLiteral("ppPrevBtn"));
    m_ppPrevBtn->setFixedSize(kPpCtrlBtn, kPpCtrlBtn);
    m_ppPrevBtn->setIconSize(QSize(kPpCtrlIcon, kPpCtrlIcon));
    m_ppPrevBtn->setProperty("ppInk", int(PpInk::Prev));
    m_ppPrevBtn->setCursor(Qt::PointingHandCursor);
    m_ppPrevBtn->setToolTip(I18n::instance().tr("previous"));
    connect(m_ppPrevBtn, &QPushButton::clicked, this, &PlayerPage::previousClicked);

    m_ppPlayBtn = new PlayerPageInkButton(center);
    m_ppPlayBtn->setObjectName(QStringLiteral("ppPlayBtn"));
    m_ppPlayBtn->setFixedSize(kPpPlayBtn, kPpPlayBtn);
    m_ppPlayBtn->setIconSize(QSize(kPpPlayIcon, kPpPlayIcon));
    m_ppPlayBtn->setProperty("ppInk", int(PpInk::PlayMain));
    m_ppPlayBtn->setProperty("ppPlaying", false);
    m_ppPlayBtn->setCursor(Qt::PointingHandCursor);
    m_ppPlayBtn->setToolTip(I18n::instance().tr("play"));

    m_ppNextBtn = new PlayerPageInkButton(center);
    m_ppNextBtn->setObjectName(QStringLiteral("ppNextBtn"));
    m_ppNextBtn->setFixedSize(kPpCtrlBtn, kPpCtrlBtn);
    m_ppNextBtn->setIconSize(QSize(kPpCtrlIcon, kPpCtrlIcon));
    m_ppNextBtn->setProperty("ppInk", int(PpInk::Next));
    m_ppNextBtn->setCursor(Qt::PointingHandCursor);
    m_ppNextBtn->setToolTip(I18n::instance().tr("next"));
    connect(m_ppNextBtn, &QPushButton::clicked, this, &PlayerPage::nextClicked);

    btnRow->addWidget(m_ppPrevBtn);
    btnRow->addWidget(m_ppPlayBtn);
    btnRow->addWidget(m_ppNextBtn);
    centerLay->addLayout(btnRow);

    auto *sliderRow = new QHBoxLayout();
    sliderRow->setContentsMargins(0, 0, 0, 0);
    sliderRow->setSpacing(8);
    sliderRow->setAlignment(Qt::AlignVCenter);

    m_ppCurTime = new QLabel(QStringLiteral("0:00"), center);
    m_ppCurTime->setObjectName(QStringLiteral("ppCurTime"));
    m_ppCurTime->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_ppCurTime->setFixedWidth(40);

    m_ppProgress = new PlayerProgressSlider(center);
    m_ppProgress->setObjectName(QStringLiteral("ppProgress"));
    m_ppProgress->setRange(0, 1000);
    m_ppProgress->setValue(0);
    m_ppProgress->setFixedHeight(16);
    m_ppProgress->setMaximumWidth(kPpProgressMaxW);
    m_ppProgress->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_ppDurTime = new QLabel(QStringLiteral("0:00"), center);
    m_ppDurTime->setObjectName(QStringLiteral("ppDurTime"));
    m_ppDurTime->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_ppDurTime->setFixedWidth(40);

    sliderRow->addWidget(m_ppCurTime);
    sliderRow->addWidget(m_ppProgress, 1);
    sliderRow->addWidget(m_ppDurTime);
    centerLay->addLayout(sliderRow);

    barLay->addWidget(center, 0, Qt::AlignCenter);
    barLay->addStretch(1);

    if (auto *mainLayout = qobject_cast<QVBoxLayout *>(layout())) {
        mainLayout->setContentsMargins(40, 16, 40, 8);
        mainLayout->addWidget(m_controlBar, 0);
    }
}

void PlayerPage::connectPlayerControlEngine()
{
    if (!m_engine)
        return;

    connect(m_ppPlayBtn, &QPushButton::clicked, this, [this]() {
        if (m_engine->isActuallyPlaying())
            m_engine->fadeOut();
        else
            m_engine->fadeIn();
    });
    connect(m_engine, &PlayerEngine::stateChanged, this, &PlayerPage::updatePlayControlState);
    updatePlayControlState();

    connect(m_engine, &PlayerEngine::positionChanged, this, [this](qint64 pos) {
        if (m_ppCurTime)
            m_ppCurTime->setText(formatPlaybackTime(pos));
        if (!m_ppProgress || m_ppProgress->isSliderDown())
            return;
        const qint64 dur = m_engine->duration();
        if (dur > 0)
            m_ppProgress->setValue(static_cast<int>(pos * 1000 / dur));
    });
    connect(m_engine, &PlayerEngine::durationChanged, this, [this](qint64 dur) {
        if (m_ppDurTime)
            m_ppDurTime->setText(formatPlaybackTime(dur));
    });
    connect(m_ppProgress, &QSlider::sliderReleased, this, [this]() {
        if (!m_engine || m_engine->duration() <= 0)
            return;
        const qint64 position = static_cast<qint64>(m_ppProgress->value()) * m_engine->duration() / 1000;
        m_engine->setPosition(position);
    });
}

void PlayerPage::updatePlayControlState()
{
    if (!m_ppPlayBtn || !m_engine)
        return;
    const bool playing = m_engine->isActuallyPlaying();
    if (m_ppPlayBtn->property("ppPlaying").toBool() != playing) {
        m_ppPlayBtn->setProperty("ppPlaying", playing);
        m_ppPlayBtn->update();
    }
    m_ppPlayBtn->setToolTip(I18n::instance().tr(playing ? "pause" : "play"));
}

void PlayerPage::applyMetaLabelFonts()
{
    if (!m_titleLabel || !m_artistLabel || !m_albumLabel)
        return;

    QFont titleFont = m_titleLabel->font();
    titleFont.setPixelSize(26);
    titleFont.setWeight(QFont::Bold);
    m_titleLabel->setFont(titleFont);

    QFont artistFont = m_artistLabel->font();
    artistFont.setPixelSize(16);
    artistFont.setWeight(QFont::Normal);
    m_artistLabel->setFont(artistFont);
    if (m_albumLabel)
        m_albumLabel->setFont(artistFont);
}

void PlayerPage::setupUi()
{
    setObjectName("playerPage");

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 16, 40, 32);
    mainLayout->setSpacing(0);

    m_backBtn = new QPushButton(this);
    m_backBtn->setFixedSize(44, 44);
    m_backBtn->setCursor(Qt::PointingHandCursor);
    m_backBtn->setObjectName("playerBackBtn");
    m_backBtn->setText(QString::fromUtf8("\xe2\x86\x90"));
    connect(m_backBtn, &QPushButton::clicked, this, [this]() { emit backRequested(); });

    auto *topBar = new QHBoxLayout();
    topBar->setContentsMargins(0, 0, 0, 8);
    topBar->addWidget(m_backBtn);
    topBar->addStretch();
    mainLayout->addLayout(topBar);

    auto *contentRow = new QHBoxLayout();
    contentRow->setSpacing(0);
    contentRow->setContentsMargins(0, 0, 0, 0);

    m_leftPanel = new QWidget(this);
    m_leftPanel->setObjectName(QStringLiteral("playerLeftPanel"));
    m_leftPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_leftPanel->setMinimumWidth(280);

    auto *leftOuter = new QVBoxLayout(m_leftPanel);
    leftOuter->setContentsMargins(0, 0, 24, 0);
    leftOuter->setSpacing(0);
    leftOuter->setAlignment(Qt::AlignCenter);

    m_leftInfoColumn = new QWidget(m_leftPanel);
    m_leftInfoColumn->setObjectName(QStringLiteral("playerLeftInfoColumn"));
    m_leftInfoColumn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    auto *infoLay = new QVBoxLayout(m_leftInfoColumn);
    infoLay->setContentsMargins(0, 0, 0, 0);
    infoLay->setSpacing(24);
    infoLay->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    m_coverLabel = new QLabel(m_leftInfoColumn);
    m_coverLabel->setFixedSize(kPlayerCoverSize, kPlayerCoverSize);
    m_coverLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_coverLabel->setScaledContents(false);
    m_coverLabel->setAlignment(Qt::AlignCenter);
    m_coverLabel->setObjectName("playerCoverLabel");
    infoLay->addWidget(m_coverLabel, 0, Qt::AlignHCenter);

    m_metaPanel = new QWidget(m_leftInfoColumn);
    m_metaPanel->setObjectName(QStringLiteral("playerMetaPanel"));
    m_metaPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    auto *metaLay = new QVBoxLayout(m_metaPanel);
    metaLay->setContentsMargins(0, 0, 0, 0);
    metaLay->setSpacing(12);
    metaLay->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    m_titleLabel = new QLabel(I18n::instance().tr("unknown"), m_metaPanel);
    m_titleLabel->setObjectName("playerSongTitleLabel");
    m_titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_titleLabel->setWordWrap(false);
    m_titleLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    m_artistRow = new QWidget(m_metaPanel);
    auto *artistLay = new QHBoxLayout(m_artistRow);
    artistLay->setContentsMargins(0, 0, 0, 0);
    artistLay->setSpacing(6);
    auto *artistIcon = new QLabel(m_artistRow);
    artistIcon->setObjectName(QStringLiteral("playerMetaIcon"));
    artistIcon->setText(QStringLiteral("♪"));
    artistIcon->setFixedWidth(20);
    m_artistLabel = new QLabel(I18n::instance().tr("unknownArtist"), m_artistRow);
    m_artistLabel->setObjectName("playerArtistLabel");
    m_artistLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_artistLabel->setWordWrap(false);
    m_artistLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    artistLay->addWidget(artistIcon, 0, Qt::AlignTop);
    artistLay->addWidget(m_artistLabel, 1);

    m_albumRow = new QWidget(m_metaPanel);
    auto *albumLay = new QHBoxLayout(m_albumRow);
    albumLay->setContentsMargins(0, 0, 0, 0);
    albumLay->setSpacing(6);
    auto *albumIcon = new QLabel(m_albumRow);
    albumIcon->setObjectName(QStringLiteral("playerMetaIcon"));
    albumIcon->setText(QStringLiteral("◎"));
    albumIcon->setFixedWidth(20);
    m_albumLabel = new QLabel(m_albumRow);
    m_albumLabel->setObjectName("playerAlbumLabel");
    m_albumLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_albumLabel->setWordWrap(false);
    m_albumLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    albumLay->addWidget(albumIcon, 0, Qt::AlignTop);
    albumLay->addWidget(m_albumLabel, 1);

    m_fullMetaTitle = m_titleLabel->text();
    m_fullMetaArtist = m_artistLabel->text();
    m_fullMetaAlbum.clear();

    metaLay->addWidget(m_titleLabel);
    metaLay->addWidget(m_artistRow);
    metaLay->addWidget(m_albumRow);
    infoLay->addWidget(m_metaPanel, 0, Qt::AlignLeft);

    m_videoStatusLbl = new QLabel(m_leftInfoColumn);
    m_videoStatusLbl->setObjectName(QStringLiteral("playerVideoStatusLbl"));
    m_videoStatusLbl->setWordWrap(true);
    m_videoStatusLbl->hide();

    m_videoRenderBtn = new QPushButton(I18n::instance().tr(QStringLiteral("videoRenderButton")), m_leftInfoColumn);
    m_videoRenderBtn->setObjectName(QStringLiteral("playerVideoRenderBtn"));
    m_videoRenderBtn->setCursor(Qt::PointingHandCursor);
    connect(m_videoRenderBtn, &QPushButton::clicked, this, &PlayerPage::openVideoRenderDialog);

    m_videoDownloadBtn = new QPushButton(I18n::instance().tr(QStringLiteral("videoRenderDownload")), m_leftInfoColumn);
    m_videoDownloadBtn->setObjectName(QStringLiteral("playerVideoDownloadBtn"));
    m_videoDownloadBtn->setCursor(Qt::PointingHandCursor);
    m_videoDownloadBtn->hide();
    connect(m_videoDownloadBtn, &QPushButton::clicked, this, &PlayerPage::downloadRenderedVideo);

    infoLay->addWidget(m_videoStatusLbl, 0, Qt::AlignLeft);
    infoLay->addWidget(m_videoRenderBtn, 0, Qt::AlignLeft);
    infoLay->addWidget(m_videoDownloadBtn, 0, Qt::AlignLeft);

    m_videoPollTimer = new QTimer(this);
    m_videoPollTimer->setInterval(8000);
    connect(m_videoPollTimer, &QTimer::timeout, this, &PlayerPage::pollVideoRenderStatus);

    leftOuter->addStretch(1);
    leftOuter->addWidget(m_leftInfoColumn, 0, Qt::AlignCenter);
    leftOuter->addStretch(1);

    m_rightPanel = new QWidget(this);
    m_rightPanel->setObjectName(QStringLiteral("playerRightPanel"));
    m_rightPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_rightPanel->setMinimumWidth(320);

    auto *lyricsCol = new QVBoxLayout(m_rightPanel);
    lyricsCol->setContentsMargins(kLyricPadLeft, 0, 16, 0);
    lyricsCol->setSpacing(0);

    m_lyricsScroll = new QScrollArea(m_rightPanel);
    m_lyricsScroll->setObjectName("lyricsScroll");
    m_lyricsScroll->setWidgetResizable(true);
    m_lyricsScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_lyricsScroll->setFrameShape(QFrame::NoFrame);

    m_lyricsContainer = new QWidget();
    m_lyricsContainer->setObjectName(QStringLiteral("lyricsContainer"));
    m_lyricsLayout = new QVBoxLayout(m_lyricsContainer);
    m_lyricsLayout->setAlignment(Qt::AlignTop);
    m_lyricsLayout->setSpacing(16);
    m_lyricsLayout->setContentsMargins(0, 0, 16, 0);

    m_lyricsScroll->setWidget(m_lyricsContainer);
    nekoPolishScrollAreaViewport(m_lyricsScroll);
    lyricsCol->addWidget(m_lyricsScroll, 1);

    contentRow->addWidget(m_leftPanel, kPlayerStyleRatio);
    contentRow->addWidget(m_rightPanel, 100 - kPlayerStyleRatio);

    mainLayout->addLayout(contentRow, 1);

    setupPlayerControl();
    connectPlayerControlEngine();

    applyPlayerPageStyle();
    applyMetaTextElide();
    relayoutLeftInfoColumn();
}

void PlayerPage::setMusicInfo(int id, const QString &title, const QString &artist,
                              const QString &album, const QString &coverUrl)
{
    const QString t = title.isEmpty() ? I18n::instance().tr("unknown") : title;
    const QString a = artist.isEmpty() ? I18n::instance().tr("unknownArtist") : artist;
    const int prevId = m_musicId;
    const QString prevCoverUrl = m_coverUrl;
    m_musicId = id;
    if (prevId != id)
        resetVideoRenderState();
    m_trackDurationSec = qMax(1, trackDurationSec());
    m_fullMetaTitle = t;
    m_fullMetaArtist = a;
    m_fullMetaAlbum = album;
    m_titleIsPlaceholder = title.isEmpty();
    m_artistIsPlaceholder = artist.isEmpty();
    applyMetaTextElide();

    disconnect(m_coverConn);
    m_coverConn = {};

    if (m_musicId < 0) {
        m_coverUrl = coverUrl;
        const QString fu = CoverCache::resolveCoverUrl(coverUrl);
        if (fu.startsWith(QLatin1String("file:"), Qt::CaseInsensitive)) {
            QPixmap px;
            if (px.load(QUrl(fu).toLocalFile()))
                applyCoverPixmap(px);
            else
                applyCoverUnknownLarge();
        } else {
            applyCoverUnknownLarge();
        }
        updateVideoRenderUi();
        return;
    }

    if (m_musicId <= 0) {
        m_coverLabel->clear();
        updateVideoRenderUi();
        return;
    }
    // 同一首歌且封面 URL 未变：不重复走 CoverCache / 网络
    if (prevId == id && coverUrl == prevCoverUrl)
        return;

    m_coverUrl = coverUrl;
    loadCover(coverUrl);
    updateVideoRenderUi();
}

void PlayerPage::retranslate()
{
    if (m_titleIsPlaceholder)
        m_fullMetaTitle = I18n::instance().tr("unknown");
    if (m_artistIsPlaceholder)
        m_fullMetaArtist = I18n::instance().tr("unknownArtist");
    applyMetaTextElide();

    // Update no lyrics message if visible
    auto *noLyricsText = findChild<QLabel *>("noLyricsText");
    if (noLyricsText) {
        noLyricsText->setText(I18n::instance().tr("noLyrics"));
    }

    if (m_videoRenderBtn)
        m_videoRenderBtn->setText(I18n::instance().tr(QStringLiteral("videoRenderButton")));
    if (m_videoDownloadBtn)
        m_videoDownloadBtn->setText(I18n::instance().tr(QStringLiteral("videoRenderDownload")));
    updateVideoRenderUi();
}

static int labelLineHeight(const QLabel *label, const QString &text, int width)
{
    if (!label)
        return 0;
    label->ensurePolished();
    const QFontMetrics fm(label->font());
    const QRect bound = fm.boundingRect(0, 0, width, 512, Qt::TextSingleLine | Qt::AlignLeft, text);
    return qMax(fm.height(), bound.height());
}

void PlayerPage::applyMetaTextElide()
{
    if (!m_titleLabel || !m_artistLabel || !m_albumLabel)
        return;

    int w = kPlayerCoverSize;
    if (m_leftPanel && m_leftPanel->width() > kPlayerCoverSize + 40)
        w = qMax(kPlayerCoverSize, m_leftPanel->width() - 8);

    m_titleLabel->ensurePolished();
    m_artistLabel->ensurePolished();
    m_albumLabel->ensurePolished();

    const QFontMetrics fmTitle(m_titleLabel->font());
    const QFontMetrics fmArtist(m_artistLabel->font());
    const QFontMetrics fmAlbum(m_albumLabel->font());

    const QString elidedTitle = fmTitle.elidedText(m_fullMetaTitle, Qt::ElideRight, w);
    const QString elidedArtist = fmArtist.elidedText(m_fullMetaArtist, Qt::ElideRight, w);
    const QString elidedAlbum = fmAlbum.elidedText(m_fullMetaAlbum, Qt::ElideRight, w);

    m_titleLabel->setText(elidedTitle);
    m_artistLabel->setText(elidedArtist);
    m_albumLabel->setText(elidedAlbum);

    const int titleH = labelLineHeight(m_titleLabel, elidedTitle, w);
    const int artistH = labelLineHeight(m_artistLabel, elidedArtist, w);
    const int albumH = m_fullMetaAlbum.isEmpty() ? 0 : labelLineHeight(m_albumLabel, elidedAlbum, w);

    m_titleLabel->setFixedSize(w, titleH);
    m_artistLabel->setFixedSize(qMax(0, w - 26), artistH);
    if (albumH > 0) {
        m_albumLabel->setFixedSize(qMax(0, w - 26), albumH);
        m_albumRow->show();
    } else {
        m_albumRow->hide();
    }
    if (m_artistRow)
        m_artistRow->setFixedHeight(artistH);

    m_titleLabel->setToolTip(m_fullMetaTitle != elidedTitle ? m_fullMetaTitle : QString());
    m_artistLabel->setToolTip(m_fullMetaArtist != elidedArtist ? m_fullMetaArtist : QString());
    m_albumLabel->setToolTip(m_fullMetaAlbum != elidedAlbum ? m_fullMetaAlbum : QString());

    relayoutLeftInfoColumn();
}

void PlayerPage::relayoutLeftInfoColumn()
{
    if (!m_metaPanel || !m_leftInfoColumn)
        return;

    const int titleH = m_titleLabel ? m_titleLabel->height() : 0;
    const int artistH = m_artistRow ? m_artistRow->height() : 0;
    const int albumH = (m_albumRow && m_albumRow->isVisible()) ? m_albumRow->height() : 0;
    const int metaW = m_titleLabel ? m_titleLabel->width() : kPlayerCoverSize;
    const int metaH = titleH + artistH + albumH + 24;
    m_metaPanel->setFixedSize(metaW, metaH);

    m_leftInfoColumn->setFixedWidth(qMax(kPlayerCoverSize, metaW));
    m_leftInfoColumn->adjustSize();
}

void PlayerPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    applyMetaTextElide();

    if (m_lyricsScroll && m_lyricsContainer) {
        const int half = qMax(100, m_lyricsScroll->viewport()->height() / 2);
        if (auto *top = m_lyricsContainer->findChild<QWidget *>(QStringLiteral("lyricPadTop")))
            top->setFixedHeight(half);
        if (auto *bot = m_lyricsContainer->findChild<QWidget *>(QStringLiteral("lyricPadBottom")))
            bot->setFixedHeight(half);
    }
}

void PlayerPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    applyMetaTextElide();
    QTimer::singleShot(0, this, [this]() {
        applyMetaTextElide();
        relayoutLeftInfoColumn();
    });

    // 入场动画：淡入 + 内容上移
    auto *opacity = new QGraphicsOpacityEffect(this);
    opacity->setOpacity(0.0);
    setGraphicsEffect(opacity);

    auto *fadeIn = new QPropertyAnimation(opacity, "opacity");
    fadeIn->setDuration(350);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::OutCubic);
    fadeIn->start(QAbstractAnimation::DeleteWhenStopped);

    connect(fadeIn, &QPropertyAnimation::finished, this, [this]() {
        setGraphicsEffect(nullptr);
    });
}

void PlayerPage::applyCoverPixmap(const QPixmap &sourcePixmap)
{
    if (sourcePixmap.isNull())
        return;
    const int s = kPlayerCoverSize;
    QPixmap rounded(s, s);
    rounded.fill(Qt::transparent);
    QPainter p(&rounded);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addRoundedRect(0, 0, s, s, kPlayerCoverRadius, kPlayerCoverRadius);
    p.setClipPath(path);
    p.drawPixmap(0, 0, sourcePixmap.scaled(s, s, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_coverLabel->setPixmap(rounded);
}

void PlayerPage::applyCoverUnknownLarge()
{
    const bool dark = Theme::ThemeManager::instance().isDarkMode();
    const int s = kPlayerCoverSize;
    QPixmap pm(s, s);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPainterPath path;
    path.addRoundedRect(0, 0, s, s, kPlayerCoverRadius, kPlayerCoverRadius);
    p.fillPath(path, dark ? QColor(52, 44, 72) : QColor(236, 232, 248));
    p.setPen(dark ? QColor(230, 57, 80, 220) : QColor(111, 66, 193, 200));
    QFont f = p.font();
    f.setPixelSize(56);
    f.setWeight(QFont::DemiBold);
    p.setFont(f);
    p.drawText(pm.rect(), Qt::AlignCenter, I18n::instance().tr(QStringLiteral("unknown")));
    p.end();
    m_coverLabel->setPixmap(pm);
}

void PlayerPage::loadCover(const QString &url)
{
    if (m_musicId <= 0)
        return;

    const QString cacheKey = QString::number(m_musicId);
    QString fetchUrl = CoverCache::resolveCoverUrl(url);
    if (fetchUrl.isEmpty()) {
        fetchUrl = QString::fromUtf8("%1/api/music/cover/%2").arg(Theme::kApiBase).arg(m_musicId);
    }

    CoverCache *cc = CoverCache::instance();
    if (QPixmap cached = cc->get(cacheKey); !cached.isNull()) {
        applyCoverPixmap(cached);
        return;
    }

    m_coverLabel->clear();

    disconnect(m_coverConn);
    const int expectId = m_musicId;
    m_coverConn = connect(cc, &CoverCache::coverLoaded, this,
                            [this, cacheKey, expectId](const QString &id, const QPixmap &pix) {
                                if (id != cacheKey)
                                    return;
                                if (m_musicId != expectId)
                                    return;
                                if (pix.isNull())
                                    return;
                                applyCoverPixmap(pix);
                            });
    cc->fetchCover(cacheKey, fetchUrl);
}

void PlayerPage::loadLyricsForTrack(const MusicInfo &info)
{
    ++m_lyricsFetchGeneration;
    const int lyricsGen = m_lyricsFetchGeneration;
    m_currentLyricLine = -1;

    if (info.isLocalFile()) {
        const int cacheKey = info.id;
        if (m_lyricsCache.contains(cacheKey)) {
            m_lyrics = m_lyricsCache.value(cacheKey);
            rebuildLyricLabels();
            return;
        }

        m_lyrics.clear();
        rebuildLyricLabels();

        QString raw = EmbeddedLyrics::readEmbeddedLyricsText(info.localPath);
        if (raw.trimmed().isEmpty()) {
            const QFileInfo fi(info.localPath);
            const QString lrcPath = fi.absolutePath() + QLatin1Char('/') + fi.completeBaseName()
                + QLatin1String(".lrc");
            QFile f(lrcPath);
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QByteArray bytes = f.readAll();
                f.close();
                raw = QString::fromUtf8(bytes);
            }
        }

        if (!raw.trimmed().isEmpty())
            applyLyricsRawText(raw);
        rebuildLyricLabels();

        if (!m_lyrics.isEmpty()) {
            constexpr int kMax = 64;
            if (m_lyricsCache.size() >= kMax && !m_lyricsCache.contains(cacheKey))
                m_lyricsCache.remove(m_lyricsCache.constBegin().key());
            m_lyricsCache.insert(cacheKey, m_lyrics);
        }
        return;
    }

    const int musicId = info.id;
    if (musicId <= 0) {
        m_lyrics.clear();
        rebuildLyricLabels();
        return;
    }

    if (m_lyricsCache.contains(musicId)) {
        m_lyrics = m_lyricsCache.value(musicId);
        rebuildLyricLabels();
        return;
    }

    m_lyrics.clear();
    rebuildLyricLabels();

    QNetworkAccessManager *nam = new QNetworkAccessManager(this);
    QString url = QString::fromUtf8("%1/api/music/lyrics/%2")
        .arg(QString::fromUtf8(Theme::kApiBase))
        .arg(musicId);
    QNetworkReply *reply = nam->get(QNetworkRequest(QUrl(url)));
    connect(reply, &QNetworkReply::finished, this, [this, reply, nam, musicId, lyricsGen]() {
        const auto cleanup = [&]() {
            reply->deleteLater();
            nam->deleteLater();
        };
        if (lyricsGen != m_lyricsFetchGeneration) {
            cleanup();
            return;
        }
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(response);
            QJsonObject obj = doc.object();
            bool success = obj.value("success").toBool();
            if (success) {
                QString lrc = obj.value("data").toString();
                if (!lrc.isEmpty()) {
                    parseLrc(lrc);
                    rebuildLyricLabels();
                    if (!m_lyrics.isEmpty()) {
                        constexpr int kMax = 64;
                        if (m_lyricsCache.size() >= kMax && !m_lyricsCache.contains(musicId))
                            m_lyricsCache.remove(m_lyricsCache.constBegin().key());
                        m_lyricsCache.insert(musicId, m_lyrics);
                    }
                } else {
                    qDebug() << "歌词API返回空歌词内容，musicId:" << musicId << "，协议:"
                             << httpProtocolLabel(reply);
                }
            } else {
                qDebug() << "歌词API返回失败，musicId:" << musicId << "，协议:" << httpProtocolLabel(reply)
                         << "，message:" << obj.value("message").toString();
            }
        } else {
            qDebug() << "歌词API请求失败，musicId:" << musicId << "，协议:" << httpProtocolLabel(reply)
                     << "，error:" << reply->errorString();
        }
        cleanup();
    });
}

void PlayerPage::applyLyricsRawText(const QString &raw)
{
    QString t = raw;
    if (!t.isEmpty() && t.front() == QChar(0xFEFF))
        t.remove(0, 1);
    parseLrc(t);
    if (m_lyrics.isEmpty() && !t.trimmed().isEmpty()) {
        static const QRegularExpression hasTime(R"(\[\d+:\d)");
        if (!t.contains(hasTime))
            m_lyrics.append({0, t.trimmed(), QString()});
    }
}

void PlayerPage::parseLrc(const QString &lrc)
{
    m_lyrics.clear();
    // 支持 [m:s]、小数 1～5 位（与后端一致）、一行多时间轴；小数按 F/10^L 秒四舍五入为毫秒
    static const QRegularExpression timeRe(R"(\[(\d+):(\d{1,2})(?:\.(\d{1,5}))?\])");
    static const QRegularExpression transRe(R"(^\{["'](.+)["']\}$)");

    auto subsecondToMs = [](const QString &frac) -> int {
        if (frac.isEmpty())
            return 0;
        bool ok = false;
        const int F = frac.toInt(&ok);
        if (!ok || F < 0)
            return 0;
        const int L = frac.length();
        if (L < 1 || L > 5)
            return 0;
        static const qint64 kPow10[] = {1, 10, 100, 1000, 10000, 100000};
        const qint64 denom = kPow10[L];
        return static_cast<int>((qint64{F} * 1000 + denom / 2) / denom);
    };

    const QStringList lines = lrc.split('\n');
    for (int i = 0; i < lines.size(); ++i) {
        const QString line = lines[i].trimmed();
        if (line.isEmpty())
            continue;

        QRegularExpressionMatchIterator tagIt = timeRe.globalMatch(line);
        if (!tagIt.hasNext())
            continue;

        QString plain = line;
        plain.remove(timeRe);
        plain = plain.trimmed();

        QString translation;
        if (i + 1 < lines.size()) {
            const QString next = lines[i + 1].trimmed();
            const auto tMatch = transRe.match(next);
            if (tMatch.hasMatch())
                translation = tMatch.captured(1);
        }

        tagIt = timeRe.globalMatch(line);
        while (tagIt.hasNext()) {
            const QRegularExpressionMatch match = tagIt.next();
            const int min = match.captured(1).toInt();
            const int sec = match.captured(2).toInt();
            const int ms = subsecondToMs(match.captured(3));
            const qint64 timeMs = (static_cast<qint64>(min) * 60 + sec) * 1000 + ms;
            m_lyrics.append({timeMs, plain, translation});
        }
    }

    std::sort(m_lyrics.begin(), m_lyrics.end(),
              [](const LyricLine &a, const LyricLine &b) { return a.time < b.time; });
}

void PlayerPage::rebuildLyricLabels()
{
    QLayoutItem *item;
    while ((item = m_lyricsLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    if (m_lyrics.isEmpty()) {
        auto *noData = new QLabel(QString::fromUtf8("\xe2\x99\xaa"), m_lyricsContainer);
        noData->setAlignment(Qt::AlignCenter);
        noData->setObjectName("noLyricsIcon");
        noData->setStyleSheet(QString::fromUtf8(
            "color: %1; font-size: 56px; background: transparent; margin-top: 60px;"
        ).arg(m_clrLyricDim));
        m_lyricsLayout->addWidget(noData);

        auto *noDataLabel = new QLabel(I18n::instance().tr("noLyrics"), m_lyricsContainer);
        noDataLabel->setAlignment(Qt::AlignCenter);
        noDataLabel->setObjectName("noLyricsText");
        noDataLabel->setStyleSheet(QString::fromUtf8(
            "color: %1; font-size: 14px; background: transparent;"
        ).arg(m_clrLyricDim));
        m_lyricsLayout->addWidget(noDataLabel);
        m_lyricsLayout->addStretch();
        emitDesktopLyricsPayload();
        emitBarLyricUpdate(-1);
        return;
    }

    auto *topPad = new QWidget(m_lyricsContainer);
    topPad->setObjectName(QStringLiteral("lyricPadTop"));
    topPad->setFixedHeight(120);
    topPad->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_lyricsLayout->addWidget(topPad);

    for (int i = 0; i < m_lyrics.size(); ++i) {
        auto *lineWidget = new QWidget(m_lyricsContainer);
        lineWidget->setObjectName(QString("lyricWidget_%1").arg(i));
        auto *lineLayout = new QVBoxLayout(lineWidget);
        lineLayout->setContentsMargins(0, 4, 0, 4);
        lineLayout->setSpacing(2);

        auto *textLabel = new QLabel(m_lyrics[i].text, lineWidget);
        textLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        textLabel->setObjectName("lyricText");
        textLabel->setProperty("lyricIndex", i);
        textLabel->setWordWrap(true);
        textLabel->setStyleSheet(QString::fromUtf8(
            "color: %1; font-size: 16px; font-weight: normal; "
            "background: transparent; padding: 0;"
        ).arg(m_clrLyricDim));
        lineLayout->addWidget(textLabel);

        if (!m_lyrics[i].translation.isEmpty()) {
            auto *transLabel = new QLabel(m_lyrics[i].translation, lineWidget);
            transLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            transLabel->setObjectName("lyricTranslation");
            transLabel->setProperty("lyricIndex", i);
            transLabel->setWordWrap(true);
            transLabel->setStyleSheet(QString::fromUtf8(
                "color: %1; font-size: 14px; background: transparent; padding: 0;"
            ).arg(m_clrLyricDim));
            lineLayout->addWidget(transLabel);
        }

        m_lyricsLayout->addWidget(lineWidget);
    }

    auto *botPad = new QWidget(m_lyricsContainer);
    botPad->setObjectName(QStringLiteral("lyricPadBottom"));
    botPad->setFixedHeight(200);
    botPad->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_lyricsLayout->addWidget(botPad);
    emitDesktopLyricsPayload();
    emitBarLyricUpdate(-1);
}

void PlayerPage::emitBarLyricUpdate(int lineIndex)
{
    QString text;
    if (lineIndex >= 0 && lineIndex < m_lyrics.size()) {
        const LyricLine &line = m_lyrics.at(lineIndex);
        text = line.text;
        const QString trans = line.translation.trimmed();
        if (!trans.isEmpty())
            text += QStringLiteral("（ %1 ）").arg(trans);
    }
    emit barLyricLineChanged(text, lineIndex, !m_lyrics.isEmpty());
}

QString PlayerPage::serializeLyricsForDesktop() const
{
    if (m_lyrics.isEmpty())
        return {};

    QString out;
    for (const LyricLine &line : m_lyrics) {
        const qint64 t = line.time;
        const int min = static_cast<int>(t / 60000);
        const int sec = static_cast<int>((t / 1000) % 60);
        const int cs = static_cast<int>((t % 1000) / 10);
        out += QStringLiteral("[%1:%2.%3]%4\n")
                   .arg(min)
                   .arg(sec, 2, 10, QChar('0'))
                   .arg(cs, 2, 10, QChar('0'))
                   .arg(line.text);
    }
    return out;
}

void PlayerPage::emitDesktopLyricsPayload()
{
    emit lyricsPayloadReady(serializeLyricsForDesktop());
}

void PlayerPage::updateLyricHighlight(qint64 positionMs)
{
    if (m_lyrics.isEmpty()) {
        if (m_currentLyricLine != -1) {
            m_currentLyricLine = -1;
            emitBarLyricUpdate(-1);
        }
        return;
    }

    int line = -1;
    for (int i = m_lyrics.size() - 1; i >= 0; --i) {
        if (positionMs >= m_lyrics.at(i).time) {
            line = i;
            break;
        }
    }

    if (line == m_currentLyricLine)
        return;
    m_currentLyricLine = line;
    emitBarLyricUpdate(line);

    for (int i = 0; i < m_lyricsLayout->count(); ++i) {
        QLayoutItem *layoutItem = m_lyricsLayout->itemAt(i);
        if (!layoutItem) continue;
        auto *widget = qobject_cast<QWidget *>(layoutItem->widget());
        if (!widget) continue;

        auto *textLabel = widget->findChild<QLabel *>("lyricText");
        auto *transLabel = widget->findChild<QLabel *>("lyricTranslation");
        if (!textLabel) continue;

        int idx = textLabel->property("lyricIndex").toInt();
        bool isCurrent = (idx == line);

        textLabel->setStyleSheet(QString::fromUtf8(
            "color: %1; font-size: %2px; font-weight: %3; "
            "background: transparent; padding: 0;"
        ).arg(isCurrent ? m_clrLyricHi : m_clrLyricDim)
         .arg(isCurrent ? 22 : 16)
         .arg(isCurrent ? "bold" : "normal"));

        if (transLabel) {
            transLabel->setStyleSheet(QString::fromUtf8(
                "color: %1; font-size: %2px; background: transparent; padding: 0;"
            ).arg(isCurrent ? m_clrLyricHiTrans : m_clrLyricDim)
             .arg(isCurrent ? 16 : 14));
        }
    }

    if (line >= 0 && m_lyricsScroll) {
        QWidget *lineWidget = nullptr;
        for (int i = 0; i < m_lyricsLayout->count(); ++i) {
            QLayoutItem *it = m_lyricsLayout->itemAt(i);
            if (!it || !it->widget())
                continue;
            auto *textLabel = it->widget()->findChild<QLabel *>(QStringLiteral("lyricText"));
            if (textLabel && textLabel->property("lyricIndex").toInt() == line) {
                lineWidget = it->widget();
                break;
            }
        }
        if (lineWidget) {
            const int y = lineWidget->mapTo(m_lyricsContainer, QPoint(0, 0)).y();
            const int viewH = m_lyricsScroll->viewport()->height();
            const int lineH = lineWidget->height();
            const int target = qMax(0, y - qMax(0, (viewH - lineH) / 2));
            auto *scrollBar = m_lyricsScroll->verticalScrollBar();

            if (m_scrollAnim) {
                m_scrollAnim->stop();
                delete m_scrollAnim;
                m_scrollAnim = nullptr;
            }

            m_scrollAnim = new QPropertyAnimation(scrollBar, "value", this);
            m_scrollAnim->setDuration(300);
            m_scrollAnim->setStartValue(scrollBar->value());
            m_scrollAnim->setEndValue(target);
            m_scrollAnim->setEasingCurve(QEasingCurve::OutCubic);
            connect(m_scrollAnim, &QPropertyAnimation::finished, this, [this]() {
                m_scrollAnim = nullptr;
            });
            m_scrollAnim->start(QAbstractAnimation::DeleteWhenStopped);
        }
    }
}

int PlayerPage::trackDurationSec() const
{
    const qint64 ms = m_engine ? m_engine->duration() : 0;
    if (ms > 0)
        return static_cast<int>(ms / 1000);
    return qMax(1, m_trackDurationSec);
}

void PlayerPage::resetVideoRenderState()
{
    m_videoJobId.clear();
    m_videoJobStatus.clear();
    m_videoJobError.clear();
    m_videoRemainingToday = -1;
    m_videoRenderBusy = false;
    if (m_videoPollTimer)
        m_videoPollTimer->stop();
    updateVideoRenderUi();
}

void PlayerPage::updateVideoRenderUi()
{
    const bool online = m_musicId > 0;
    if (m_videoRenderBtn) {
        m_videoRenderBtn->setVisible(online);
        m_videoRenderBtn->setEnabled(online && !m_videoRenderBusy
                                     && m_videoJobStatus != QStringLiteral("pending")
                                     && m_videoJobStatus != QStringLiteral("processing"));
        if (m_videoRenderBusy)
            m_videoRenderBtn->setText(I18n::instance().tr(QStringLiteral("videoRenderButtonBusy")));
        else
            m_videoRenderBtn->setText(I18n::instance().tr(QStringLiteral("videoRenderButton")));
    }
    if (m_videoDownloadBtn)
        m_videoDownloadBtn->setVisible(online && m_videoJobStatus == QStringLiteral("done"));
    if (!m_videoStatusLbl)
        return;
    if (!online || m_videoJobStatus.isEmpty()) {
        m_videoStatusLbl->hide();
        return;
    }
    QString text;
    if (m_videoJobStatus == QStringLiteral("pending") || m_videoJobStatus == QStringLiteral("processing")) {
        text = I18n::instance().tr(QStringLiteral("videoRenderStatusProcessing"));
        if (m_videoRemainingToday >= 0)
            text += QLatin1Char('\n')
                    + I18n::instance().tr(QStringLiteral("videoRenderRemainingToday"))
                          .arg(m_videoRemainingToday);
    } else if (m_videoJobStatus == QStringLiteral("done")) {
        text = I18n::instance().tr(QStringLiteral("videoRenderStatusDone"));
    } else if (m_videoJobStatus == QStringLiteral("failed")) {
        const QString err = m_videoJobError.isEmpty() ? QStringLiteral("—") : m_videoJobError;
        text = I18n::instance().tr(QStringLiteral("videoRenderFailed")).arg(err);
    }
    if (text.isEmpty()) {
        m_videoStatusLbl->hide();
        return;
    }
    m_videoStatusLbl->setText(text);
    m_videoStatusLbl->show();
}

void PlayerPage::pollVideoRenderStatus()
{
    if (!m_apiClient || m_videoJobId.isEmpty() || !UserManager::instance().isLoggedIn())
        return;
    m_apiClient->fetchVideoRenderStatus(m_videoJobId, [this](bool ok, const QVariantMap &data) {
        if (!ok)
            return;
        m_videoJobStatus = data.value(QStringLiteral("status")).toString();
        m_videoJobError = data.value(QStringLiteral("error")).toString();
        if (m_videoJobStatus == QStringLiteral("done") || m_videoJobStatus == QStringLiteral("failed")) {
            if (m_videoPollTimer)
                m_videoPollTimer->stop();
        }
        updateVideoRenderUi();
    });
}

void PlayerPage::openVideoRenderDialog()
{
    if (m_musicId <= 0) {
        Toast::show(window(), I18n::instance().tr(QStringLiteral("videoRenderLocalUnsupported")), Toast::Info);
        return;
    }
    if (!UserManager::instance().isLoggedIn()) {
        Toast::show(window(), I18n::instance().tr(QStringLiteral("pleaseLoginFirst")), Toast::Error);
        return;
    }
    if (!m_apiClient)
        return;
    if (m_videoJobStatus == QStringLiteral("failed"))
        resetVideoRenderState();

    VideoRenderDialog dlg(m_apiClient, m_musicId, m_fullMetaTitle, m_fullMetaArtist, trackDurationSec(), this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    m_videoRenderBusy = true;
    updateVideoRenderUi();
    m_apiClient->createVideoRenderJob(m_musicId, dlg.startSec(), dlg.watermarked(),
                                      [this](bool ok, const QString &message, const QVariantMap &data) {
                                          m_videoRenderBusy = false;
                                          if (!ok) {
                                              Toast::show(window(),
                                                          message.isEmpty()
                                                              ? I18n::instance().tr(QStringLiteral(
                                                                    "videoRenderCreateFailed"))
                                                              : message,
                                                          Toast::Error);
                                              updateVideoRenderUi();
                                              return;
                                          }
                                          m_videoJobId = data.value(QStringLiteral("jobId")).toString();
                                          m_videoJobStatus =
                                              data.value(QStringLiteral("status")).toString();
                                          if (m_videoJobStatus.isEmpty())
                                              m_videoJobStatus = QStringLiteral("pending");
                                          if (data.contains(QStringLiteral("remainingToday")))
                                              m_videoRemainingToday =
                                                  data.value(QStringLiteral("remainingToday")).toInt();
                                          Toast::show(window(),
                                                      I18n::instance().tr(QStringLiteral("videoRenderSubmitted")),
                                                      Toast::Success, 4500);
                                          updateVideoRenderUi();
                                          if (m_videoPollTimer && !m_videoJobId.isEmpty())
                                              m_videoPollTimer->start();
                                          pollVideoRenderStatus();
                                      });
}

void PlayerPage::downloadRenderedVideo()
{
    if (m_videoJobId.isEmpty() || !m_apiClient)
        return;
    const QString safeTitle = m_fullMetaTitle;
    QString base = safeTitle;
    base.replace(QRegularExpression(QStringLiteral(R"([/\\?%*:|"<>])")), QStringLiteral("_"));
    if (base.isEmpty())
        base = QStringLiteral("clip");
    const QString defaultName = base + QStringLiteral(".mp4");
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    const QString path = QFileDialog::getSaveFileName(
        this, I18n::instance().tr(QStringLiteral("videoRenderDownload")), dir + QLatin1Char('/') + defaultName,
        QStringLiteral("MP4 (*.mp4)"));
    if (path.isEmpty())
        return;

    m_apiClient->downloadVideoRenderFile(m_videoJobId, path, [this, path](bool ok, const QString &err) {
        if (ok) {
            Toast::show(window(), I18n::instance().tr(QStringLiteral("videoRenderDownloadDone")).arg(path),
                      Toast::Success, 5000);
        } else {
            Toast::show(window(),
                        I18n::instance().tr(QStringLiteral("videoRenderFailed"))
                            .arg(err.isEmpty() ? I18n::instance().tr(QStringLiteral("downloadFailed")) : err),
                        Toast::Error);
        }
    });
}
