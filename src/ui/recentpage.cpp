/**
 * @file recentpage.cpp
 * @brief "最近播放"页面 — 显示最近播放的 100 条音乐记录
 */

#include "recentpage.h"
#include "core/i18n.h"
#include "core/playlistdb.h"
#include "core/covercache.h"
#include "core/musicinfo.h"
#include "theme/theme.h"
#include "theme/thememanager.h"
#include "ui/svgicon.h"
#include "ui/scrollareafix.h"

#include <QSizePolicy>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QMenu>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

// ─── 单曲封面标签（圆角 6px + 异步加载）─────────────────
class RecentCoverLabel : public QLabel
{
public:
    explicit RecentCoverLabel(int size, QWidget *parent = nullptr) : QLabel(parent), m_size(size)
    {
        setFixedSize(size, size);
        setScaledContents(false);
        setPlaceholder();
    }

    void setPlaceholder()
    {
        m_pixmap = QPixmap(m_size, m_size);
        m_pixmap.fill(Qt::transparent);
        QPainter p(&m_pixmap);
        QPainterPath pp;
        pp.addRoundedRect(0, 0, m_size, m_size, 6, 6);
        p.fillPath(pp, QColor(128, 128, 128, 40));
        p.setClipPath(pp);
        auto iconPx = Icons::render(Icons::kMusic, 28, QColor(255, 255, 255, 100));
        p.drawPixmap((m_size - 28) / 2, (m_size - 28) / 2, iconPx);
        update();
    }

    void loadCover(const QString &url)
    {
        if (url.isEmpty()) { setPlaceholder(); return; }
        QString musicId = url.mid(url.lastIndexOf(QLatin1Char('/')) + 1);

        QPixmap cached = CoverCache::instance()->get(musicId);
        if (!cached.isNull()) {
            applyPixmap(cached);
            return;
        }

        connect(CoverCache::instance(), &CoverCache::coverLoaded, this,
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
        int s = qMin(pix.width(), pix.height());
        m_pixmap = pix.copy((pix.width()-s)/2, (pix.height()-s)/2, s, s)
            .scaled(m_size, m_size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        update();
    }

    QPixmap m_pixmap;
    int m_size;
};

// ─── 最近播放音乐卡片 ────────────────────────
class RecentMusicCard : public QWidget
{
public:
    explicit RecentMusicCard(const MusicInfo &info, QWidget *parent = nullptr)
        : QWidget(parent), m_musicId(info.id), m_info(info)
    {
        setFixedHeight(70);
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_StyledBackground, false);

        auto *lay = new QHBoxLayout(this);
        lay->setContentsMargins(12, 8, 12, 8);
        lay->setSpacing(14);

        // 封面
        auto *coverLbl = new RecentCoverLabel(54, this);
        coverLbl->loadCover(info.coverUrl);
        lay->addWidget(coverLbl);

        // 信息
        auto *infoV = new QWidget(this);
        auto *infoLay = new QVBoxLayout(infoV);
        infoLay->setContentsMargins(0, 0, 0, 0);
        infoLay->setSpacing(4);

        auto *titleRow = new QWidget(infoV);
        titleRow->setAttribute(Qt::WA_TranslucentBackground);
        auto *titleRowLay = new QHBoxLayout(titleRow);
        titleRowLay->setContentsMargins(0, 0, 0, 0);
        titleRowLay->setSpacing(8);

        auto *titleLbl = new QLabel(info.title, titleRow);
        titleLbl->setStyleSheet("QLabel { font-size: 14px; font-weight: 600; color: " + QString(Theme::kTextMain) + "; }");
        titleLbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        titleRowLay->addWidget(titleLbl, 1);

        if (info.isLocalFile()) {
            auto *localBadge = new QLabel(I18n::instance().tr(QStringLiteral("localMusicBadge")), titleRow);
            localBadge->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
            const bool dark = Theme::ThemeManager::instance().isDarkMode();
            const QString fg = dark ? QString::fromUtf8(Theme::kLavenderLt) : QStringLiteral("#6F42C1");
            const QString bg = dark ? QStringLiteral("rgba(230,57,80,0.18)") : QStringLiteral("rgba(111,66,193,0.12)");
            const QString bd = dark ? QStringLiteral("rgba(230,57,80,0.45)") : QStringLiteral("rgba(111,66,193,0.35)");
            localBadge->setStyleSheet(QStringLiteral(
                "QLabel { font-size: 10px; font-weight: 700; color: %1; padding: 2px 8px; border-radius: 6px; "
                "background: %2; border: 1px solid %3; }")
                                          .arg(fg, bg, bd));
            titleRowLay->addWidget(localBadge, 0, Qt::AlignRight | Qt::AlignVCenter);
        }
        infoLay->addWidget(titleRow);

        auto *artistLbl = new QLabel(info.artist, infoV);
        artistLbl->setStyleSheet("QLabel { font-size: 12px; color: " + QString(Theme::kTextSub) + "; }");
        infoLay->addWidget(artistLbl);

        infoLay->addStretch();
        lay->addWidget(infoV, 1);

        // 时长：始终占位一列，避免部分有条目有、部分无导致不齐（duration 为 0 时多为写入最近播放时尚未带上时长）
        QString timeText;
        if (info.duration > 0) {
            const int totalMins = info.duration / 60;
            const int secs = info.duration % 60;
            timeText = QStringLiteral("%1:%2")
                           .arg(totalMins, 2, 10, QChar('0'))
                           .arg(secs, 2, 10, QChar('0'));
        } else {
            timeText = QStringLiteral("--:--");
        }
        auto *timeLbl = new QLabel(timeText, this);
        timeLbl->setMinimumWidth(52);
        timeLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        timeLbl->setStyleSheet("QLabel { font-size: 12px; color: " + QString(Theme::kTextMuted) + "; }");
        lay->addWidget(timeLbl);
    }

    int musicId() const { return m_musicId; }
    const MusicInfo& info() const { return m_info; }

    std::function<void(int)> onClicked;
    std::function<void(int)> onRemove;

protected:
    void paintEvent(QPaintEvent *event) override
    {
        QWidget::paintEvent(event);
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QPainterPath path;
        path.addRoundedRect(rect().adjusted(2, 2, -2, -2), 8, 8);
        p.fillPath(path, QColor(45, 38, 65, 100));
    }

    void mousePressEvent(QMouseEvent *e) override
    {
        if (e->button() == Qt::LeftButton && onClicked) {
            onClicked(m_musicId);
        }
        QWidget::mousePressEvent(e);
    }

    void contextMenuEvent(QContextMenuEvent *event) override
    {
        QMenu menu(this);
        menu.setStyleSheet(
            "QMenu { background-color: rgba(40, 40, 50, 0.95); border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 8px; padding: 4px; }"
            "QMenu::item { color: #e0e0e0; padding: 8px 24px; border-radius: 4px; }"
            "QMenu::item:selected { background-color: rgba(255, 255, 255, 0.1); }"
        );

        QAction *removeAction = menu.addAction(I18n::instance().tr("remove"));
        QAction *selected = menu.exec(event->globalPos());
        if (selected == removeAction && onRemove) {
            onRemove(m_musicId);
        }
    }

private:
    int m_musicId;
    MusicInfo m_info;
};

// ─── RecentPage ────────────────────────────────────────

RecentPage::RecentPage(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, false);
    setAutoFillBackground(false);
    setupUi();

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

void RecentPage::retranslate()
{
    // Update title if needed
}

void RecentPage::refresh()
{
    loadRecentPlays();
}

void RecentPage::setupUi()
{
    m_scroll = new QScrollArea(this);
    m_scroll->setWidgetResizable(true);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setObjectName("recentScroll");

    m_container = new QWidget(m_scroll);
    m_container->setObjectName("recentContainer");
    m_mainLay = new QVBoxLayout(m_container);
    m_mainLay->setContentsMargins(24, 24, 24, 24);
    m_mainLay->setSpacing(16);

    // 标题
    auto *titleLabel = new QLabel(I18n::instance().tr("recentPlay"), m_container);
    titleLabel->setObjectName("recentTitle");
    m_mainLay->addWidget(titleLabel);

    // 列表区域
    m_listLay = new QVBoxLayout();
    m_listLay->setContentsMargins(0, 0, 0, 0);
    m_listLay->setSpacing(8);
    m_listLay->setAlignment(Qt::AlignTop);
    m_mainLay->addLayout(m_listLay);

    m_mainLay->addStretch();

    m_scroll->setWidget(m_container);
    nekoPolishScrollAreaViewport(m_scroll);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(m_scroll);

    // 加载数据
    loadRecentPlays();
}

void RecentPage::loadRecentPlays()
{
    QLayoutItem *item;
    while ((item = m_listLay->takeAt(0)) != nullptr) {
        if (QWidget *w = item->widget())
            w->deleteLater();
        delete item;
    }

    auto recentPlays = PlaylistDatabase::instance().getRecentPlays();

    if (recentPlays.isEmpty()) {
        auto *emptyLbl = new QLabel(I18n::instance().tr("emptyRecent"), m_container);
        emptyLbl->setObjectName("recentEmpty");
        emptyLbl->setAlignment(Qt::AlignCenter);
        emptyLbl->setStyleSheet("QLabel { font-size: 14px; color: " + QString(Theme::kTextMuted) + "; padding: 40px 0; }");
        m_listLay->addWidget(emptyLbl);
        m_listLay->addStretch(1);
        return;
    }

    for (const auto &info : recentPlays) {
        auto *card = new RecentMusicCard(info, m_container);
        card->onClicked = [this, info](int musicId) {
            Q_UNUSED(musicId);
            emit playRequested(info);
        };
        card->onRemove = [this](int musicId) {
            // For now, we don't have a remove single item method, so just refresh
            // In the future we could add a removeRecentPlay method
            Q_UNUSED(musicId);
        };
        m_listLay->addWidget(card);
    }
    m_listLay->addStretch(1);
}

void RecentPage::paintEvent(QPaintEvent *)
{
    // 透明，由父窗口渐变背景透出
}
