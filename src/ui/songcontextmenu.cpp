/**
 * @file songcontextmenu.cpp
 * @brief 歌曲右键菜单 — 自定义 Popup 面板 + 图标行
 */

#include "songcontextmenu.h"
#include "theme/theme.h"
#include "theme/thememanager.h"
#include "ui/svgicon.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QApplication>
#include <QScreen>

namespace {

constexpr int kMenuMinW = 200;
constexpr int kMenuMaxW = 280;
constexpr int kRowH = 44;
constexpr int kPad = 6;
constexpr int kIconSize = 18;

class SongContextMenuRow final : public QWidget
{
public:
    explicit SongContextMenuRow(const SongContextMenuPopup::Entry &entry, QWidget *parent = nullptr)
        : QWidget(parent), m_entry(entry)
    {
        setFixedHeight(kRowH);
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_StyledBackground, true);

        auto *lay = new QHBoxLayout(this);
        lay->setContentsMargins(12, 0, 14, 0);
        lay->setSpacing(10);

        m_iconLbl = new QLabel(this);
        m_iconLbl->setFixedSize(kIconSize, kIconSize);
        m_iconLbl->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        lay->addWidget(m_iconLbl);

        m_textLbl = new QLabel(entry.label, this);
        m_textLbl->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        lay->addWidget(m_textLbl, 1);
    }

    const SongContextMenuPopup::Entry &entry() const { return m_entry; }

    void applyTheme();

protected:
    void enterEvent(QEnterEvent *) override
    {
        m_hovered = true;
        update();
    }

    void leaveEvent(QEvent *) override
    {
        m_hovered = false;
        update();
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            if (m_entry.action)
                m_entry.action();
            if (auto *popup = window())
                popup->close();
        }
        event->accept();
    }

    void paintEvent(QPaintEvent *) override
    {
        if (!m_hovered)
            return;
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        const bool dark = Theme::ThemeManager::instance().isDarkMode();
        QColor bg = dark ? QColor(255, 255, 255, 28) : QColor(230, 57, 80, 28);
        p.setPen(Qt::NoPen);
        p.setBrush(bg);
        p.drawRoundedRect(rect().adjusted(4, 2, -4, -2), 8, 8);
    }

private:
    SongContextMenuPopup::Entry m_entry;
    QLabel *m_iconLbl = nullptr;
    QLabel *m_textLbl = nullptr;
    bool m_hovered = false;
};

void SongContextMenuRow::applyTheme()
{
    const bool dark = Theme::ThemeManager::instance().isDarkMode();
    const QString fg = dark ? QString::fromUtf8(Theme::kTextMain) : QStringLiteral("#212529");
    const QColor ic = dark ? QColor(244, 246, 255, 210) : QColor(33, 37, 41, 210);
    if (m_iconLbl && m_entry.iconName)
        m_iconLbl->setPixmap(Icons::renderNamed(m_entry.iconName, kIconSize, ic));
    if (m_textLbl) {
        m_textLbl->setStyleSheet(
            QStringLiteral("QLabel { color: %1; font-size: 14px; font-weight: 500; }").arg(fg));
    }
}

} // namespace

void SongContextMenuPopup::showAt(QWidget *anchor, const QPoint &globalPos, const QList<Entry> &entries)
{
    if (!anchor || entries.isEmpty())
        return;

    auto *menu = new SongContextMenuPopup(anchor, entries);
    menu->positionAt(globalPos);
    menu->show();
    menu->raise();
    menu->activateWindow();
}

SongContextMenuPopup::SongContextMenuPopup(QWidget *anchor, const QList<Entry> &entries)
    : QWidget(anchor, Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
    , m_anchor(anchor)
    , m_entries(entries)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setAttribute(Qt::WA_TranslucentBackground, true);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(8, 8, 8, 8);
    outer->setSpacing(0);

    m_panel = new QWidget(this);
    m_panel->setObjectName(QStringLiteral("songContextMenuPanel"));
    auto *shadow = new QGraphicsDropShadowEffect(m_panel);
    shadow->setBlurRadius(24);
    shadow->setOffset(0, 6);
    shadow->setColor(QColor(0, 0, 0, 70));
    m_panel->setGraphicsEffect(shadow);

    auto *panelLay = new QVBoxLayout(m_panel);
    panelLay->setContentsMargins(kPad, kPad, kPad, kPad);
    panelLay->setSpacing(2);

    int maxTextW = 0;
    QFontMetrics fm(font());
    for (const Entry &e : entries) {
        maxTextW = qMax(maxTextW, fm.horizontalAdvance(e.label));
    }
    const int w = qBound(kMenuMinW, maxTextW + 12 + kIconSize + 10 + 14 + 14, kMenuMaxW);
    setFixedWidth(w + 16);

    for (const Entry &e : entries) {
        auto *row = new SongContextMenuRow(e, m_panel);
        row->applyTheme();
        panelLay->addWidget(row);
        m_rowWidgets.append(row);
    }

    outer->addWidget(m_panel);
    setFixedHeight(entries.size() * kRowH + 2 * kPad + 16);

    applyTheme();

    connect(&Theme::ThemeManager::instance(), &Theme::ThemeManager::themeChanged, this,
            [this]() { applyTheme(); });
}

void SongContextMenuPopup::applyTheme()
{
    const bool dark = Theme::ThemeManager::instance().isDarkMode();
    if (!m_panel)
        return;
    if (dark) {
        m_panel->setStyleSheet(QStringLiteral(
            "QWidget#songContextMenuPanel {"
            "  background: rgb(40, 40, 44);"
            "  border: 1px solid rgba(255, 255, 255, 0.1);"
            "  border-radius: 12px;"
            "}"));
    } else {
        m_panel->setStyleSheet(QStringLiteral(
            "QWidget#songContextMenuPanel {"
            "  background: #ffffff;"
            "  border: 1px solid rgba(0, 0, 0, 0.08);"
            "  border-radius: 12px;"
            "}"));
    }

    for (QWidget *w : m_rowWidgets)
        static_cast<SongContextMenuRow *>(w)->applyTheme();
}

void SongContextMenuPopup::positionAt(const QPoint &globalPos)
{
    QScreen *screen = m_anchor && m_anchor->window() ? m_anchor->window()->screen() : QApplication::primaryScreen();
    if (!screen)
        screen = QApplication::primaryScreen();
    QRect avail = screen ? screen->availableGeometry() : QRect(0, 0, 1920, 1080);

    int x = globalPos.x();
    int y = globalPos.y();
    if (x + width() > avail.right())
        x = avail.right() - width();
    if (y + height() > avail.bottom())
        y = globalPos.y() - height();
    if (x < avail.left())
        x = avail.left();
    if (y < avail.top())
        y = avail.top();

    move(x, y);
}
