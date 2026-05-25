/**
 * @file playlistcard.cpp
 * @brief 歌单卡片（网格版）实现
 *
 * paintEvent 绘制：圆角裁剪封面 + 底部渐变遮罩 + 歌曲数。
 * 悬停平滑上浮 4px + 樱花粉描边。
 */

#include "playlistcard.h"
#include "theme/theme.h"
#include "ui/svgicon.h"
#include "core/i18n.h"
#include "core/covercache.h"

#include <QVBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QEnterEvent>
#include <QPropertyAnimation>

PlaylistCard::PlaylistCard(const PlaylistInfo &info, QWidget *parent)
    : QWidget(parent), m_info(info)
{
    setFixedSize(182, 236);
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_StyledBackground, false);

    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(6, 6, 6, 6);
    lay->setSpacing(8);

    // 封面占位（由 paintEvent 绘制实际图片）
    auto *coverSpacer = new QWidget(this);
    coverSpacer->setFixedSize(Theme::kCoverSmall, Theme::kCoverSmall);
    lay->addWidget(coverSpacer, 0, Qt::AlignHCenter);

    // 歌单名
    m_name = new QLabel(this);
    m_name->setObjectName("plName");
    m_name->setFixedWidth(168);
    QFontMetrics fm(m_name->font());
    m_name->setText(fm.elidedText(m_info.name, Qt::ElideRight, 168));
    m_name->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    lay->addWidget(m_name);

    loadCover();
}

int PlaylistCard::playlistId() const { return m_info.id; }

void PlaylistCard::loadCover()
{
    // 渐变占位封面
    QPixmap ph(Theme::kCoverSmall, Theme::kCoverSmall);
    ph.fill(Qt::transparent);
    QPainter p(&ph);
    QPainterPath pp;
    pp.addRoundedRect(0, 0, Theme::kCoverSmall, Theme::kCoverSmall,
                      Theme::kCoverRadius, Theme::kCoverRadius);
    QLinearGradient g(0, 0, Theme::kCoverSmall, Theme::kCoverSmall);
    g.setColorAt(0.0, QColor(255, 107, 139));
    g.setColorAt(1.0, QColor(214, 40, 57));
    p.fillPath(pp, g);
    p.setPen(Qt::white);
    p.drawPixmap(QRect((Theme::kCoverSmall-40)/2, (Theme::kCoverSmall-40)/2, 40, 40),
                 Icons::renderNamed("Music", 40, QColor(255, 255, 255, 160)));
    p.end();
    m_coverPixmap = ph;

    // 异步加载封面（先查缓存）
    if (!m_info.coverUrl.isEmpty()) {
        QString musicId = m_info.coverUrl.mid(m_info.coverUrl.lastIndexOf(QLatin1Char('/')) + 1);

        QPixmap cached = CoverCache::instance()->get(musicId);
        if (!cached.isNull()) {
            int s = qMin(cached.width(), cached.height());
            m_coverPixmap = cached.copy((cached.width()-s)/2, (cached.height()-s)/2, s, s)
                .scaled(Theme::kCoverSmall, Theme::kCoverSmall,
                        Qt::KeepAspectRatioByExpanding, Qt::FastTransformation);
            update();
            return;
        }

        disconnect(m_coverConn);
        m_coverConn = connect(CoverCache::instance(), &CoverCache::coverLoaded, this,
                [this, musicId](const QString &id, const QPixmap &pix) {
            if (id == musicId) {
                int s = qMin(pix.width(), pix.height());
                m_coverPixmap = pix.copy((pix.width()-s)/2, (pix.height()-s)/2, s, s)
                    .scaled(Theme::kCoverSmall, Theme::kCoverSmall,
                            Qt::KeepAspectRatioByExpanding, Qt::FastTransformation);
                update();
            }
        });
        CoverCache::instance()->fetchCover(musicId, m_info.coverUrl);
    }
}

void PlaylistCard::animateHover(bool enter)
{
    auto *anim = new QPropertyAnimation(this, "pos");
    anim->setDuration(200);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    QPoint cur = pos();
    anim->setStartValue(cur);
    anim->setEndValue(enter ? QPoint(cur.x(), cur.y() - 4) : QPoint(cur.x(), cur.y() + 4));
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void PlaylistCard::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 圆角裁剪封面
    QRect coverRect(6, 6, Theme::kCoverSmall, Theme::kCoverSmall);
    QPainterPath clip;
    clip.addRoundedRect(coverRect, Theme::kCoverRadius, Theme::kCoverRadius);
    p.setClipPath(clip);
    p.drawPixmap(coverRect, m_coverPixmap);

    // 底部渐变遮罩
    QLinearGradient mask(coverRect.bottomLeft(), coverRect.topLeft());
    mask.setColorAt(0.0, QColor(14, 14, 28, 190));
    mask.setColorAt(0.35, QColor(14, 14, 28, 85));
    mask.setColorAt(1.0, QColor(14, 14, 28, 0));
    p.fillRect(coverRect, mask);
    p.setClipping(false);

    // 歌曲数量
    if (m_info.musicCount > 0) {
        p.setPen(QColor(245, 240, 255, 200));
        p.setFont(QFont(QString(), 10, QFont::Bold));
        QRect countRect = coverRect.adjusted(0, 0, -8, -6);
        p.drawText(countRect, Qt::AlignRight | Qt::AlignBottom,
                   QString::number(m_info.musicCount) + I18n::instance().tr("musicCount"));
    }

    if (m_hovered) {
        QPainterPath border;
        border.addRoundedRect(rect().adjusted(1, 1, -1, -1), Theme::kRMd, Theme::kRMd);
        p.setPen(QPen(QColor(255, 183, 197, 130), 2));
        p.drawPath(border);
    }
}

void PlaylistCard::enterEvent(QEnterEvent *e)
{
    m_hovered = true;
    animateHover(true);
    update();
    QWidget::enterEvent(e);
}

void PlaylistCard::leaveEvent(QEvent *e)
{
    m_hovered = false;
    animateHover(false);
    update();
    QWidget::leaveEvent(e);
}

void PlaylistCard::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) emit clicked(m_info.id);
    QWidget::mousePressEvent(e);
}
