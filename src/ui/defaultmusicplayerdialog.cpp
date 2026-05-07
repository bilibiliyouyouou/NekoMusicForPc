#include "defaultmusicplayerdialog.h"
#include "core/i18n.h"
#include "theme/theme.h"

#include <QCheckBox>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

/** 外层 layout 左右留白（与阴影留白一致） */
constexpr int kOuterPad = 20;
/** 内层卡片左右内边距 */
constexpr int kCardPadH = 28;
constexpr int kWinCardPadH = 26;
constexpr int kMainDialogWidth = 540;
constexpr int kWinFollowUpWidth = 500;

int textColumnWidth(int dialogWidth, int cardPadH)
{
    return dialogWidth - 2 * kOuterPad - 2 * cardPadH;
}

void polishFramelessDialog(QDialog *dlg)
{
    dlg->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
    dlg->setAttribute(Qt::WA_TranslucentBackground);
    dlg->setModal(true);

    auto *shadow = new QGraphicsDropShadowEffect(dlg);
    shadow->setBlurRadius(28);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 100));
    dlg->setGraphicsEffect(shadow);
}

} // namespace

DefaultMusicPlayerDialog::DefaultMusicPlayerDialog(QWidget *parent)
    : QDialog(parent)
{
    polishFramelessDialog(this);
    setFixedWidth(kMainDialogWidth);

    const int textW = textColumnWidth(kMainDialogWidth, kCardPadH);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(kOuterPad, kOuterPad, kOuterPad, kOuterPad);
    outer->setSpacing(0);

    auto *container = new QWidget(this);
    container->setObjectName(QStringLiteral("defaultMusicPlayerBox"));
    container->setStyleSheet(
        QStringLiteral(
            "QWidget#defaultMusicPlayerBox {"
            "  background: %1;"
            "  border: 1px solid %2;"
            "  border-radius: %3px;"
            "}")
            .arg(QString::fromUtf8(Theme::kGlassBg), QString::fromUtf8(Theme::kBorderGlass))
            .arg(Theme::kRMd));

    auto *lay = new QVBoxLayout(container);
    lay->setContentsMargins(kCardPadH, 24, kCardPadH, 22);
    lay->setSpacing(16);

    auto *titleLbl = new QLabel(I18n::instance().tr(QStringLiteral("defaultMusicPlayerTitle")), container);
    titleLbl->setWordWrap(true);
    titleLbl->setFixedWidth(textW);
    titleLbl->setStyleSheet(
        QStringLiteral("QLabel { font-size: 18px; font-weight: 700; color: %1; }")
            .arg(QString::fromUtf8(Theme::kTextMain)));
    lay->addWidget(titleLbl);

    auto *msgLbl = new QLabel(I18n::instance().tr(QStringLiteral("defaultMusicPlayerMessage")), container);
    msgLbl->setWordWrap(true);
    msgLbl->setFixedWidth(textW);
    msgLbl->setStyleSheet(
        QStringLiteral("QLabel { font-size: 14px; color: %1; line-height: 1.45; }")
            .arg(QString::fromUtf8(Theme::kTextSub)));
    lay->addWidget(msgLbl);

    m_dontAskAgain = new QCheckBox(I18n::instance().tr(QStringLiteral("defaultMusicPlayerDontAskAgain")), container);
    m_dontAskAgain->setCursor(Qt::PointingHandCursor);
    m_dontAskAgain->setStyleSheet(
        QStringLiteral(
            "QCheckBox { color: %1; font-size: 13px; spacing: 8px; }"
            "QCheckBox::indicator { width: 18px; height: 18px; border-radius: 4px;"
            "  border: 1px solid %2; background: rgba(36, 31, 49, 120); }"
            "QCheckBox::indicator:checked { background: %3; border: 1px solid %3; }")
            .arg(QString::fromUtf8(Theme::kTextSub), QString::fromUtf8(Theme::kBorderGlass),
                 QString::fromUtf8(Theme::kLavender)));
    lay->addWidget(m_dontAskAgain);

    lay->addSpacing(4);

    auto *btnRow = new QHBoxLayout();
    btnRow->setSpacing(10);

    auto *laterBtn = new QPushButton(I18n::instance().tr(QStringLiteral("defaultMusicPlayerNo")), container);
    laterBtn->setFixedHeight(40);
    laterBtn->setCursor(Qt::PointingHandCursor);
    laterBtn->setStyleSheet(
        QStringLiteral(
            "QPushButton {"
            "  background: rgba(255, 255, 255, 0.06);"
            "  color: %1;"
            "  border: 1px solid %2;"
            "  border-radius: 8px;"
            "  font-size: 14px;"
            "}"
            "QPushButton:hover { background: rgba(255, 255, 255, 0.1); }")
            .arg(QString::fromUtf8(Theme::kTextMain), QString::fromUtf8(Theme::kBorderGlass)));
    connect(laterBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnRow->addWidget(laterBtn, 1);

    auto *setBtn = new QPushButton(I18n::instance().tr(QStringLiteral("defaultMusicPlayerYes")), container);
    setBtn->setFixedHeight(40);
    setBtn->setCursor(Qt::PointingHandCursor);
    setBtn->setStyleSheet(
        QStringLiteral(
            "QPushButton {"
            "  background: %1;"
            "  color: %2;"
            "  border: none;"
            "  border-radius: 8px;"
            "  font-size: 14px; font-weight: 600;"
            "}"
            "QPushButton:hover { background: %3; }")
            .arg(QString::fromUtf8(Theme::kGradMain), QString::fromUtf8(Theme::kTextMain),
                 QString::fromUtf8(Theme::kLavenderDk)));
    connect(setBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnRow->addWidget(setBtn, 1);

    lay->addLayout(btnRow);

    outer->addWidget(container);

    adjustSize();
}

bool DefaultMusicPlayerDialog::dontAskAgain() const
{
    return m_dontAskAgain && m_dontAskAgain->isChecked();
}

void DefaultMusicPlayerDialog::showWindowsDefaultAppsFollowUp(QWidget *parent)
{
    QDialog dlg(parent);
    polishFramelessDialog(&dlg);
    dlg.setFixedWidth(kWinFollowUpWidth);

    const int textW = textColumnWidth(kWinFollowUpWidth, kWinCardPadH);

    auto *outer = new QVBoxLayout(&dlg);
    outer->setContentsMargins(kOuterPad, kOuterPad, kOuterPad, kOuterPad);

    auto *container = new QWidget(&dlg);
    container->setObjectName(QStringLiteral("defaultMusicPlayerWinBox"));
    container->setStyleSheet(
        QStringLiteral(
            "QWidget#defaultMusicPlayerWinBox {"
            "  background: %1;"
            "  border: 1px solid %2;"
            "  border-radius: %3px;"
            "}")
            .arg(QString::fromUtf8(Theme::kGlassBg), QString::fromUtf8(Theme::kBorderGlass))
            .arg(Theme::kRMd));

    auto *lay = new QVBoxLayout(container);
    lay->setContentsMargins(26, 22, 26, 20);
    lay->setSpacing(18);

    auto *titleLbl = new QLabel(I18n::instance().tr(QStringLiteral("defaultMusicPlayerTitle")), container);
    titleLbl->setWordWrap(true);
    titleLbl->setFixedWidth(textW);
    titleLbl->setStyleSheet(
        QStringLiteral("QLabel { font-size: 17px; font-weight: 700; color: %1; }")
            .arg(QString::fromUtf8(Theme::kTextMain)));
    lay->addWidget(titleLbl);

    auto *msgLbl = new QLabel(I18n::instance().tr(QStringLiteral("defaultMusicPlayerWinFollowUp")), container);
    msgLbl->setWordWrap(true);
    msgLbl->setFixedWidth(textW);
    msgLbl->setStyleSheet(
        QStringLiteral("QLabel { font-size: 14px; color: %1; }").arg(QString::fromUtf8(Theme::kTextSub)));
    lay->addWidget(msgLbl);

    auto *okBtn = new QPushButton(I18n::instance().tr(QStringLiteral("ok")), container);
    okBtn->setFixedHeight(40);
    okBtn->setCursor(Qt::PointingHandCursor);
    okBtn->setStyleSheet(
        QStringLiteral(
            "QPushButton {"
            "  background: %1;"
            "  color: %2;"
            "  border: none;"
            "  border-radius: 8px;"
            "  font-size: 14px; font-weight: 600;"
            "}"
            "QPushButton:hover { background: %3; }")
            .arg(QString::fromUtf8(Theme::kGradMain), QString::fromUtf8(Theme::kTextMain),
                 QString::fromUtf8(Theme::kLavenderDk)));
    connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    lay->addWidget(okBtn);

    outer->addWidget(container);
    dlg.adjustSize();
    dlg.exec();
}
