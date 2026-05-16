/**
 * @file vippage.cpp
 * @brief 会员中心实现
 */

#include "vippage.h"
#include "core/apiclient.h"
#include "core/i18n.h"
#include "core/usermanager.h"
#include "core/vipqrcode.h"
#include "theme/theme.h"
#include "theme/thememanager.h"
#include "ui/toast.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QUrl>
#include <QDesktopServices>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <algorithm>

namespace {

QString heroCardStyle(bool isVip)
{
    const bool dark = Theme::ThemeManager::instance().isDarkMode();
    if (isVip) {
        return dark
            ? QStringLiteral(
                  "QWidget { background: qlineargradient(x1:0,y1:0,x2:1,y2:1,"
                  "stop:0 rgba(255,248,225,0.18), stop:1 rgba(255,183,77,0.12));"
                  "border: 1px solid rgba(255, 179, 0, 0.35); border-radius: 16px; }")
            : QStringLiteral(
                  "QWidget { background: qlineargradient(x1:0,y1:0,x2:1,y2:1,"
                  "stop:0 rgba(255,248,225,0.92), stop:1 rgba(255,236,179,0.88));"
                  "border: 1px solid rgba(255, 179, 0, 0.28); border-radius: 16px; }");
    }
    return dark
        ? QStringLiteral(
              "QWidget { background: rgba(255,255,255,0.06);"
              "border: 1px solid rgba(255,255,255,0.10); border-radius: 16px; }")
        : QStringLiteral(
              "QWidget { background: rgba(255,255,255,0.55);"
              "border: 1px solid rgba(0,0,0,0.08); border-radius: 16px; }");
}

QString checkoutAsideStyle()
{
    const bool dark = Theme::ThemeManager::instance().isDarkMode();
    return dark
        ? QStringLiteral(
              "QWidget#vipCheckout { background: rgba(255,255,255,0.05);"
              "border: 1px solid rgba(255,255,255,0.10); border-radius: 16px; }")
        : QStringLiteral(
              "QWidget#vipCheckout { background: rgba(255,255,255,0.82);"
              "border: 1px solid rgba(0,0,0,0.07); border-radius: 16px; }");
}

QString planCardStyle(bool active)
{
    const bool dark = Theme::ThemeManager::instance().isDarkMode();
    if (active) {
        return QStringLiteral(
            "QPushButton { text-align: left; padding: 14px 16px; border-radius: 12px;"
            "border: 2px solid %1; background: rgba(255,107,139,0.12); }"
            "QPushButton:hover { background: rgba(255,107,139,0.18); }")
            .arg(Theme::kLavender);
    }
    return dark
        ? QStringLiteral(
              "QPushButton { text-align: left; padding: 14px 16px; border-radius: 12px;"
              "border: 1px solid rgba(255,255,255,0.10); background: rgba(255,255,255,0.04); }"
              "QPushButton:hover { background: rgba(255,255,255,0.08); }")
        : QStringLiteral(
              "QPushButton { text-align: left; padding: 14px 16px; border-radius: 12px;"
              "border: 1px solid rgba(0,0,0,0.08); background: rgba(255,255,255,0.65); }"
              "QPushButton:hover { background: rgba(255,255,255,0.92); }");
}

QString payBtnStyle(const QString &accent)
{
    return QStringLiteral(
               "QPushButton { background: %1; border: none; border-radius: 18px;"
               "color: %2; font-size: 13px; font-weight: 600; padding: 10px 16px; }"
               "QPushButton:hover { background: %3; }"
               "QPushButton:disabled { background: rgba(128,128,128,0.35); color: rgba(255,255,255,0.5); }")
        .arg(accent, QString(Theme::kBgMid), QString(Theme::kMintLt));
}

QString qrFrameStyle()
{
    const bool dark = Theme::ThemeManager::instance().isDarkMode();
    return dark
        ? QStringLiteral(
              "QLabel#vipQrFrame { background: #ffffff; border-radius: 12px;"
              "border: 1px solid rgba(255,255,255,0.15); padding: 8px; }")
        : QStringLiteral(
              "QLabel#vipQrFrame { background: #ffffff; border-radius: 12px;"
              "border: 1px solid rgba(0,0,0,0.08); padding: 8px; }");
}

} // namespace

VipPage::VipPage(ApiClient *apiClient, QWidget *parent)
    : QWidget(parent)
    , m_apiClient(apiClient)
    , m_nam(new QNetworkAccessManager(this))
{
    setAttribute(Qt::WA_StyledBackground, false);
    setAutoFillBackground(false);
    setupUi();

    connect(&UserManager::instance(), &UserManager::vipStatusChanged, this, [this]() {
        updateHero();
    });

    auto *eff = new QGraphicsOpacityEffect(this);
    eff->setOpacity(0.0);
    setGraphicsEffect(eff);
    auto *anim = new QPropertyAnimation(eff, "opacity");
    anim->setDuration(600);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    connect(anim, &QPropertyAnimation::finished, this, [this]() { setGraphicsEffect(nullptr); });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void VipPage::setupUi()
{
    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(20, 20, 20, 88);
    root->setSpacing(20);

    // ─── 左栏：状态 + 套餐 ─────────────────────────────
    m_leftScroll = new QScrollArea(this);
    m_leftScroll->setWidgetResizable(true);
    m_leftScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_leftScroll->setFrameShape(QFrame::NoFrame);

    m_leftContainer = new QWidget(m_leftScroll);
    auto *leftLay = new QVBoxLayout(m_leftContainer);
    leftLay->setContentsMargins(4, 4, 12, 4);
    leftLay->setSpacing(16);

    auto *titleRow = new QWidget(m_leftContainer);
    auto *titleRowLay = new QHBoxLayout(titleRow);
    titleRowLay->setContentsMargins(0, 0, 0, 0);
    m_titleLabel = new QLabel(I18n::instance().tr(QStringLiteral("vipCenterTitle")), titleRow);
    m_titleLabel->setStyleSheet(QStringLiteral("font-size: 20px; font-weight: 700;"));
    titleRowLay->addWidget(m_titleLabel);
    titleRowLay->addStretch();
    m_refreshBtn = new QPushButton(I18n::instance().tr(QStringLiteral("vipRefresh")), titleRow);
    m_refreshBtn->setFixedHeight(34);
    m_refreshBtn->setCursor(Qt::PointingHandCursor);
    m_refreshBtn->setStyleSheet(payBtnStyle(QString(Theme::kLavender)));
    connect(m_refreshBtn, &QPushButton::clicked, this, [this]() { refresh(); });
    titleRowLay->addWidget(m_refreshBtn);
    leftLay->addWidget(titleRow);

    m_heroCard = new QWidget(m_leftContainer);
    auto *heroLay = new QVBoxLayout(m_heroCard);
    heroLay->setContentsMargins(20, 18, 20, 18);
    heroLay->setSpacing(6);
    m_heroTitle = new QLabel(m_heroCard);
    m_heroTitle->setWordWrap(true);
    m_heroTitle->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: 700;"));
    m_heroSubtitle = new QLabel(m_heroCard);
    m_heroSubtitle->setWordWrap(true);
    m_heroSubtitle->setStyleSheet(QStringLiteral("font-size: 13px; color: rgba(200,200,220,0.9);"));
    heroLay->addWidget(m_heroTitle);
    heroLay->addWidget(m_heroSubtitle);
    leftLay->addWidget(m_heroCard);

    auto *tagline = new QLabel(I18n::instance().tr(QStringLiteral("vipTagline")), m_leftContainer);
    tagline->setWordWrap(true);
    tagline->setStyleSheet(QStringLiteral("font-size: 14px; color: rgba(200,200,220,0.85);"));
    leftLay->addWidget(tagline);

    auto *plansTitle = new QLabel(I18n::instance().tr(QStringLiteral("vipChoosePlan")), m_leftContainer);
    plansTitle->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: 600;"));
    leftLay->addWidget(plansTitle);

    m_planStripHost = new QWidget(m_leftContainer);
    m_planStripLay = new QHBoxLayout(m_planStripHost);
    m_planStripLay->setContentsMargins(0, 0, 0, 0);
    m_planStripLay->setSpacing(10);
    m_planStripLay->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    leftLay->addWidget(m_planStripHost);

    m_statusLabel = new QLabel(m_leftContainer);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->hide();
    leftLay->addWidget(m_statusLabel);

    auto *terms = new QLabel(I18n::instance().tr(QStringLiteral("vipTerms")), m_leftContainer);
    terms->setWordWrap(true);
    terms->setStyleSheet(QStringLiteral("font-size: 12px; color: rgba(180,180,200,0.75);"));
    leftLay->addWidget(terms);
    leftLay->addStretch(1);

    m_leftScroll->setWidget(m_leftContainer);
    root->addWidget(m_leftScroll, 1);

    // ─── 右栏：结算 + 二维码 ───────────────────────────
    m_checkoutAside = new QWidget(this);
    m_checkoutAside->setObjectName(QStringLiteral("vipCheckout"));
    m_checkoutAside->setFixedWidth(300);
    m_checkoutAside->setStyleSheet(checkoutAsideStyle());
    auto *checkoutLay = new QVBoxLayout(m_checkoutAside);
    checkoutLay->setContentsMargins(20, 22, 20, 22);
    checkoutLay->setSpacing(12);

    auto *checkoutLabel = new QLabel(I18n::instance().tr(QStringLiteral("vipCheckoutCurrentPlan")), m_checkoutAside);
    checkoutLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: rgba(180,180,200,0.8);"));
    checkoutLay->addWidget(checkoutLabel);

    m_checkoutPlanLabel = new QLabel(m_checkoutAside);
    m_checkoutPlanLabel->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: 600;"));
    checkoutLay->addWidget(m_checkoutPlanLabel);

    m_checkoutPriceLabel = new QLabel(m_checkoutAside);
    m_checkoutPriceLabel->setStyleSheet(
        QStringLiteral("font-size: 28px; font-weight: 800; color: %1;").arg(Theme::kLavender));
    checkoutLay->addWidget(m_checkoutPriceLabel);

    m_checkoutPlaceholder = new QLabel(I18n::instance().tr(QStringLiteral("vipCheckoutSelectPlan")), m_checkoutAside);
    m_checkoutPlaceholder->setWordWrap(true);
    m_checkoutPlaceholder->setAlignment(Qt::AlignCenter);
    checkoutLay->addWidget(m_checkoutPlaceholder);

    m_checkoutPayChoice = new QWidget(m_checkoutAside);
    auto *payChoiceLay = new QVBoxLayout(m_checkoutPayChoice);
    payChoiceLay->setContentsMargins(0, 0, 0, 0);
    payChoiceLay->setSpacing(10);
    auto *payHint = new QLabel(I18n::instance().tr(QStringLiteral("vipCheckoutPayHint")), m_checkoutPayChoice);
    payHint->setStyleSheet(QStringLiteral("font-size: 13px;"));
    payChoiceLay->addWidget(payHint);
    auto *payRow = new QHBoxLayout();
    payRow->setSpacing(10);
    auto *aliBtn = new QPushButton(I18n::instance().tr(QStringLiteral("vipPayAlipay")), m_checkoutPayChoice);
    aliBtn->setCursor(Qt::PointingHandCursor);
    aliBtn->setStyleSheet(payBtnStyle(QString(Theme::kLavender)));
    connect(aliBtn, &QPushButton::clicked, this, [this]() { startPay(QStringLiteral("alipay")); });
    auto *wxBtn = new QPushButton(I18n::instance().tr(QStringLiteral("vipPayWechat")), m_checkoutPayChoice);
    wxBtn->setCursor(Qt::PointingHandCursor);
    wxBtn->setStyleSheet(payBtnStyle(QString(Theme::kMint)));
    connect(wxBtn, &QPushButton::clicked, this, [this]() { startPay(QStringLiteral("wxpay")); });
    payRow->addWidget(aliBtn);
    payRow->addWidget(wxBtn);
    payChoiceLay->addLayout(payRow);
    checkoutLay->addWidget(m_checkoutPayChoice);
    m_checkoutPayChoice->hide();

    m_checkoutQrPanel = new QWidget(m_checkoutAside);
    auto *qrLay = new QVBoxLayout(m_checkoutQrPanel);
    qrLay->setContentsMargins(0, 0, 0, 0);
    qrLay->setSpacing(10);
    qrLay->setAlignment(Qt::AlignHCenter);

    m_checkoutQrTitle = new QLabel(m_checkoutQrPanel);
    m_checkoutQrTitle->setAlignment(Qt::AlignCenter);
    m_checkoutQrTitle->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: 600;"));
    qrLay->addWidget(m_checkoutQrTitle);

    m_checkoutQrTip = new QLabel(I18n::instance().tr(QStringLiteral("vipQrScanTip")), m_checkoutQrPanel);
    m_checkoutQrTip->setAlignment(Qt::AlignCenter);
    m_checkoutQrTip->setWordWrap(true);
    m_checkoutQrTip->setStyleSheet(QStringLiteral("font-size: 12px; color: rgba(180,180,200,0.85);"));
    qrLay->addWidget(m_checkoutQrTip);

    m_checkoutQrImage = new QLabel(m_checkoutQrPanel);
    m_checkoutQrImage->setObjectName(QStringLiteral("vipQrFrame"));
    m_checkoutQrImage->setFixedSize(236, 236);
    m_checkoutQrImage->setAlignment(Qt::AlignCenter);
    m_checkoutQrImage->setStyleSheet(qrFrameStyle());
    m_checkoutQrImage->setScaledContents(true);
    qrLay->addWidget(m_checkoutQrImage, 0, Qt::AlignHCenter);

    m_checkoutBrowserBtn = new QPushButton(I18n::instance().tr(QStringLiteral("vipOpenInBrowser")), m_checkoutQrPanel);
    m_checkoutBrowserBtn->setCursor(Qt::PointingHandCursor);
    m_checkoutBrowserBtn->setStyleSheet(
        QStringLiteral("QPushButton { color: %1; background: transparent; border: none;"
                       "font-size: 12px; text-decoration: underline; }"
                       "QPushButton:hover { color: %2; }")
            .arg(Theme::kLavender, Theme::kLavenderLt));
    m_checkoutBrowserBtn->hide();
    connect(m_checkoutBrowserBtn, &QPushButton::clicked, this, [this]() {
        if (!m_browserPayUrl.isEmpty())
            QDesktopServices::openUrl(QUrl(m_browserPayUrl));
    });
    qrLay->addWidget(m_checkoutBrowserBtn, 0, Qt::AlignHCenter);

    auto *doneRow = new QHBoxLayout();
    doneRow->setSpacing(8);
    m_checkoutDoneBtn = new QPushButton(I18n::instance().tr(QStringLiteral("vipPaidDone")), m_checkoutQrPanel);
    m_checkoutDoneBtn->setCursor(Qt::PointingHandCursor);
    m_checkoutDoneBtn->setStyleSheet(payBtnStyle(QString(Theme::kMint)));
    connect(m_checkoutDoneBtn, &QPushButton::clicked, this, [this]() {
        syncVipStatus();
        clearPayQr();
        Toast::show(this, I18n::instance().tr(QStringLiteral("vipPaidDoneHint")));
    });
    m_checkoutChangePayBtn = new QPushButton(I18n::instance().tr(QStringLiteral("vipChangePayMethod")), m_checkoutQrPanel);
    m_checkoutChangePayBtn->setCursor(Qt::PointingHandCursor);
    m_checkoutChangePayBtn->setStyleSheet(payBtnStyle(QString(Theme::kLavender)));
    connect(m_checkoutChangePayBtn, &QPushButton::clicked, this, [this]() { clearPayQr(); });
    doneRow->addWidget(m_checkoutDoneBtn);
    doneRow->addWidget(m_checkoutChangePayBtn);
    qrLay->addLayout(doneRow);
    checkoutLay->addWidget(m_checkoutQrPanel);
    m_checkoutQrPanel->hide();

    checkoutLay->addStretch(1);
    root->addWidget(m_checkoutAside, 0);

    updateHero();
    updateCheckoutPanel();
}

void VipPage::retranslate()
{
    if (m_titleLabel)
        m_titleLabel->setText(I18n::instance().tr(QStringLiteral("vipCenterTitle")));
    if (m_refreshBtn)
        m_refreshBtn->setText(I18n::instance().tr(QStringLiteral("vipRefresh")));
    if (m_checkoutQrTip)
        m_checkoutQrTip->setText(I18n::instance().tr(QStringLiteral("vipQrScanTip")));
    updateHero();
    updateCheckoutPanel();
    if (!m_loadingPricing)
        loadPricing();
}

void VipPage::refresh()
{
    updateHero();
    syncVipStatus();
    loadPricing();
}

void VipPage::updateHero()
{
    const bool isVip = UserManager::instance().isVip();
    m_heroCard->setStyleSheet(heroCardStyle(isVip));

    if (isVip) {
        m_heroTitle->setText(I18n::instance().tr(QStringLiteral("vipStatusActive")));
        const QString exp = UserManager::instance().vipExpiresAt().left(10);
        if (exp.length() == 10)
            m_heroSubtitle->setText(I18n::instance().tr(QStringLiteral("vipStatusExpires")).arg(exp));
        else
            m_heroSubtitle->setText(I18n::instance().tr(QStringLiteral("vipMemberBenefits")));
    } else {
        m_heroTitle->setText(I18n::instance().tr(QStringLiteral("vipOpenMembership")));
        m_heroSubtitle->setText(I18n::instance().tr(QStringLiteral("vipNonMemberBenefits")));
    }
}

void VipPage::syncVipStatus()
{
    if (!m_apiClient || !UserManager::instance().isLoggedIn())
        return;
    m_apiClient->syncSessionVipStatus([this](bool ok, bool) {
        if (ok)
            updateHero();
    });
}

void VipPage::loadPricing()
{
    if (!m_apiClient)
        return;
    m_loadingPricing = true;
    clearPayQr();
    m_statusLabel->setText(I18n::instance().tr(QStringLiteral("loading")));
    m_statusLabel->show();

    m_apiClient->fetchVipPricing([this](bool ok, const QString &message, const QList<QVariantMap> &items) {
        m_loadingPricing = false;
        if (ok) {
            m_statusLabel->hide();
            m_pricingRows = items;
            rebuildPlanStrip(items);
        } else {
            m_pricingRows.clear();
            m_selectedPlanId = -1;
            rebuildPlanStrip({});
            m_statusLabel->setText(message.isEmpty()
                                      ? I18n::instance().tr(QStringLiteral("vipPricingLoadFailed"))
                                      : message);
            m_statusLabel->show();
        }
        updateCheckoutPanel();
    });
}

void VipPage::rebuildPlanStrip(const QList<QVariantMap> &items)
{
    while (QLayoutItem *it = m_planStripLay->takeAt(0)) {
        if (QWidget *w = it->widget())
            w->deleteLater();
        delete it;
    }

    QList<QVariantMap> sorted = items;
    std::sort(sorted.begin(), sorted.end(), [](const QVariantMap &a, const QVariantMap &b) {
        return a.value(QStringLiteral("sortOrder")).toInt() < b.value(QStringLiteral("sortOrder")).toInt();
    });

    if (sorted.isEmpty()) {
        m_selectedPlanId = -1;
        return;
    }

    if (m_selectedPlanId < 0
        || !std::any_of(sorted.cbegin(), sorted.cend(), [this](const QVariantMap &row) {
               return row.value(QStringLiteral("id")).toInt() == m_selectedPlanId;
           })) {
        m_selectedPlanId = sorted.first().value(QStringLiteral("id")).toInt();
    }

    for (const auto &item : sorted) {
        const int pricingId = item.value(QStringLiteral("id")).toInt();
        const double priceYuan = item.value(QStringLiteral("priceYuan")).toDouble();
        const bool active = (pricingId == m_selectedPlanId);

        auto *btn = new QPushButton(m_planStripHost);
        btn->setMinimumWidth(120);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(planCardStyle(active));
        btn->setText(QStringLiteral("%1\n%2")
                       .arg(formatPlanLabel(item),
                            I18n::instance().tr(QStringLiteral("vipPriceYuan")).arg(priceYuan, 0, 'f', 2)));
        connect(btn, &QPushButton::clicked, this, [this, pricingId]() { selectPlan(pricingId); });
        m_planStripLay->addWidget(btn);
    }
    m_planStripLay->addStretch(1);
}

void VipPage::selectPlan(int pricingId)
{
    if (m_selectedPlanId == pricingId)
        return;
    m_selectedPlanId = pricingId;
    clearPayQr();
    rebuildPlanStrip(m_pricingRows);
    updateCheckoutPanel();
}

const QVariantMap *VipPage::selectedPlan() const
{
    for (const auto &row : m_pricingRows) {
        if (row.value(QStringLiteral("id")).toInt() == m_selectedPlanId)
            return &row;
    }
    return nullptr;
}

void VipPage::updateCheckoutPanel()
{
    const QVariantMap *plan = selectedPlan();
    const bool hasPlan = plan != nullptr && !m_pricingRows.isEmpty();

    m_checkoutPlanLabel->setVisible(hasPlan);
    m_checkoutPriceLabel->setVisible(hasPlan);
    m_checkoutPlaceholder->setVisible(!hasPlan);
    m_checkoutPayChoice->setVisible(hasPlan && m_checkoutQrPanel->isHidden());

    if (!hasPlan) {
        m_checkoutPlanLabel->clear();
        m_checkoutPriceLabel->clear();
        return;
    }

    m_checkoutPlanLabel->setText(formatPlanLabel(*plan));
    m_checkoutPriceLabel->setText(
        I18n::instance().tr(QStringLiteral("vipPriceYuan")).arg(plan->value(QStringLiteral("priceYuan")).toDouble(), 0, 'f', 2));
}

void VipPage::clearPayQr()
{
    if (m_qrImageReply) {
        m_qrImageReply->disconnect();
        m_qrImageReply->abort();
        m_qrImageReply->deleteLater();
        m_qrImageReply = nullptr;
    }
    m_browserPayUrl.clear();
    m_checkoutQrPanel->hide();
    m_checkoutQrImage->clear();
    m_checkoutBrowserBtn->hide();
    m_payBusyPlanId = -1;
    updateCheckoutPanel();
}

QString VipPage::formatPlanLabel(const QVariantMap &item)
{
    const int months = item.value(QStringLiteral("months")).toInt();
    const int days = item.value(QStringLiteral("days")).toInt();
    QStringList parts;
    if (months > 0)
        parts.append(QString::number(months) + I18n::instance().tr(QStringLiteral("vipMonthsSuffix")));
    if (days > 0)
        parts.append(QString::number(days) + I18n::instance().tr(QStringLiteral("vipDaysSuffix")));
    if (!parts.isEmpty())
        return parts.join(QStringLiteral(" · "));
    return I18n::instance().tr(QStringLiteral("vipPlanDefault"));
}

QString VipPage::firstNonEmpty(const QVariantMap &data, std::initializer_list<const char *> keys)
{
    for (const char *key : keys) {
        const QString v = data.value(QString::fromUtf8(key)).toString().trimmed();
        if (!v.isEmpty())
            return v;
    }
    return {};
}

void VipPage::startPay(const QString &payType)
{
    if (!UserManager::instance().isLoggedIn()) {
        Toast::show(this, I18n::instance().tr(QStringLiteral("vipLoginRequired")));
        emit loginRequested();
        return;
    }
    const QVariantMap *plan = selectedPlan();
    if (!plan || !m_apiClient)
        return;
    if (m_payBusyPlanId >= 0)
        return;

    const int pricingId = plan->value(QStringLiteral("id")).toInt();
    m_payBusyPlanId = pricingId;

    m_apiClient->createVipPayOrder(pricingId, payType, [this, payType](bool ok, const QString &message, const QVariantMap &data) {
        m_payBusyPlanId = -1;
        if (!ok) {
            Toast::show(this, message.isEmpty()
                                ? I18n::instance().tr(QStringLiteral("vipPayCreateFailed"))
                                : message);
            return;
        }
        const QString label = payType == QStringLiteral("wxpay")
            ? I18n::instance().tr(QStringLiteral("vipPayWechat"))
            : I18n::instance().tr(QStringLiteral("vipPayAlipay"));
        showCheckoutQr(data, label);
    });
}

void VipPage::showCheckoutQr(const QVariantMap &data, const QString &payLabel)
{
    const QString imageUrl = firstNonEmpty(data, {"img"});
    const QString linkForEncode = firstNonEmpty(data, {"qrcode", "payurl", "payurl2"});
    m_browserPayUrl = firstNonEmpty(data, {"payurl2", "payurl", "qrcode"});

    m_checkoutQrTitle->setText(I18n::instance().tr(QStringLiteral("vipQrPayTitle")).arg(payLabel));
    m_checkoutPayChoice->hide();
    m_checkoutQrPanel->show();
    m_checkoutPlaceholder->hide();

    if (!imageUrl.isEmpty()) {
        loadQrImageFromUrl(imageUrl, linkForEncode);
    } else if (!linkForEncode.isEmpty()) {
        const QPixmap qr = VipQrCode::pixmapFromText(linkForEncode, 220);
        if (qr.isNull()) {
            Toast::show(this, I18n::instance().tr(QStringLiteral("vipQrGenerateFailed")));
            clearPayQr();
            return;
        }
        m_checkoutQrImage->setPixmap(qr);
    } else {
        Toast::show(this, I18n::instance().tr(QStringLiteral("vipPayNoUrl")));
        clearPayQr();
        return;
    }

    m_checkoutBrowserBtn->setVisible(!m_browserPayUrl.isEmpty());
}

void VipPage::loadQrImageFromUrl(const QString &url, const QString &encodeFallback)
{
    m_checkoutQrImage->setText(I18n::instance().tr(QStringLiteral("loading")));

    if (m_qrImageReply) {
        m_qrImageReply->disconnect();
        m_qrImageReply->abort();
        m_qrImageReply->deleteLater();
        m_qrImageReply = nullptr;
    }

    QNetworkRequest req{QUrl(url)};
    req.setTransferTimeout(15000);
    m_qrImageReply = m_nam->get(req);
    connect(m_qrImageReply, &QNetworkReply::finished, this,
            [this, reply = m_qrImageReply, encodeFallback]() {
        if (reply != m_qrImageReply)
            return;
        m_qrImageReply = nullptr;
        reply->deleteLater();

        auto applyEncoded = [this, encodeFallback]() -> bool {
            if (encodeFallback.isEmpty())
                return false;
            const QPixmap qr = VipQrCode::pixmapFromText(encodeFallback, 220);
            if (qr.isNull())
                return false;
            m_checkoutQrImage->setPixmap(qr);
            return true;
        };

        if (reply->error() != QNetworkReply::NoError) {
            if (!applyEncoded()) {
                Toast::show(this, I18n::instance().tr(QStringLiteral("vipQrImageLoadFailed")));
                m_checkoutQrImage->clear();
            }
            return;
        }

        QPixmap pix;
        if (!pix.loadFromData(reply->readAll())) {
            if (!applyEncoded()) {
                Toast::show(this, I18n::instance().tr(QStringLiteral("vipQrImageLoadFailed")));
                m_checkoutQrImage->clear();
            }
            return;
        }
        m_checkoutQrImage->setPixmap(pix.scaled(220, 220, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    });
}
