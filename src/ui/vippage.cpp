/**
 * @file vippage.cpp
 * @brief 会员中心实现
 */

#include "vippage.h"
#include "core/apiclient.h"
#include "core/i18n.h"
#include "core/usermanager.h"
#include "theme/theme.h"
#include "theme/thememanager.h"
#include "ui/scrollareafix.h"
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
#include <QVariantMap>
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

QString pricingCardStyle()
{
    const bool dark = Theme::ThemeManager::instance().isDarkMode();
    return dark
        ? QStringLiteral(
              "QWidget { background: rgba(255,255,255,0.05);"
              "border: 1px solid rgba(255,255,255,0.08); border-radius: 14px; }")
        : QStringLiteral(
              "QWidget { background: rgba(255,255,255,0.72);"
              "border: 1px solid rgba(0,0,0,0.06); border-radius: 14px; }");
}

QString payBtnStyle(const QString &accent)
{
    return QStringLiteral(
               "QPushButton { background: %1; border: none; border-radius: 18px;"
               "color: %2; font-size: 13px; font-weight: 600; padding: 0 16px; }"
               "QPushButton:hover { background: %3; }"
               "QPushButton:disabled { background: rgba(128,128,128,0.35); color: rgba(255,255,255,0.5); }")
        .arg(accent, QString(Theme::kBgMid), QString(Theme::kMintLt));
}

} // namespace

VipPage::VipPage(ApiClient *apiClient, QWidget *parent)
    : QWidget(parent)
    , m_apiClient(apiClient)
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
    m_scroll = new QScrollArea(this);
    m_scroll->setWidgetResizable(true);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setObjectName(QStringLiteral("vipScroll"));

    m_container = new QWidget(m_scroll);
    m_container->setObjectName(QStringLiteral("vipContainer"));
    m_mainLay = new QVBoxLayout(m_container);
    m_mainLay->setContentsMargins(24, 24, 24, 88);
    m_mainLay->setSpacing(16);

    auto *titleRow = new QWidget(m_container);
    auto *titleRowLay = new QHBoxLayout(titleRow);
    titleRowLay->setContentsMargins(0, 0, 0, 0);
    titleRowLay->setSpacing(12);

    m_titleLabel = new QLabel(I18n::instance().tr(QStringLiteral("vipCenterTitle")), titleRow);
    m_titleLabel->setObjectName(QStringLiteral("vipTitle"));
    titleRowLay->addWidget(m_titleLabel);
    titleRowLay->addStretch();

    m_refreshBtn = new QPushButton(I18n::instance().tr(QStringLiteral("vipRefresh")), titleRow);
    m_refreshBtn->setFixedHeight(36);
    m_refreshBtn->setCursor(Qt::PointingHandCursor);
    m_refreshBtn->setStyleSheet(payBtnStyle(QString(Theme::kLavender)));
    connect(m_refreshBtn, &QPushButton::clicked, this, [this]() { refresh(); });
    titleRowLay->addWidget(m_refreshBtn);
    m_mainLay->addWidget(titleRow);

    m_heroCard = new QWidget(m_container);
    m_heroCard->setObjectName(QStringLiteral("vipHero"));
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
    m_mainLay->addWidget(m_heroCard);

    auto *plansTitle = new QLabel(I18n::instance().tr(QStringLiteral("vipChoosePlan")), m_container);
    plansTitle->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: 600;"));
    m_mainLay->addWidget(plansTitle);

    m_pricingHost = new QWidget(m_container);
    m_pricingLay = new QVBoxLayout(m_pricingHost);
    m_pricingLay->setContentsMargins(0, 0, 0, 0);
    m_pricingLay->setSpacing(12);
    m_mainLay->addWidget(m_pricingHost);

    m_statusLabel = new QLabel(m_container);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->hide();
    m_mainLay->addWidget(m_statusLabel);
    m_mainLay->addStretch(1);

    m_scroll->setWidget(m_container);
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->addWidget(m_scroll);

    updateHero();
}

void VipPage::retranslate()
{
    if (m_titleLabel)
        m_titleLabel->setText(I18n::instance().tr(QStringLiteral("vipCenterTitle")));
    if (m_refreshBtn)
        m_refreshBtn->setText(I18n::instance().tr(QStringLiteral("vipRefresh")));
    updateHero();
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
    m_statusLabel->setText(I18n::instance().tr(QStringLiteral("loading")));
    m_statusLabel->show();

    m_apiClient->fetchVipPricing([this](bool ok, const QString &message, const QList<QVariantMap> &items) {
        m_loadingPricing = false;
        if (ok) {
            m_statusLabel->hide();
            rebuildPricingList(items);
        } else {
            m_statusLabel->setText(message.isEmpty()
                                      ? I18n::instance().tr(QStringLiteral("vipPricingLoadFailed"))
                                      : message);
            m_statusLabel->show();
            rebuildPricingList({});
        }
    });
}

void VipPage::rebuildPricingList(const QList<QVariantMap> &items)
{
    while (QLayoutItem *it = m_pricingLay->takeAt(0)) {
        if (QWidget *w = it->widget())
            w->deleteLater();
        delete it;
    }

    QList<QVariantMap> sorted = items;
    std::sort(sorted.begin(), sorted.end(), [](const QVariantMap &a, const QVariantMap &b) {
        return a.value(QStringLiteral("sortOrder")).toInt() < b.value(QStringLiteral("sortOrder")).toInt();
    });

    for (const auto &item : sorted) {
        const int pricingId = item.value(QStringLiteral("id")).toInt();
        const double priceYuan = item.value(QStringLiteral("priceYuan")).toDouble();

        auto *card = new QWidget(m_pricingHost);
        card->setStyleSheet(pricingCardStyle());
        auto *cardLay = new QVBoxLayout(card);
        cardLay->setContentsMargins(16, 14, 16, 14);
        cardLay->setSpacing(10);

        auto *top = new QHBoxLayout();
        auto *name = new QLabel(formatPlanLabel(item), card);
        name->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: 600;"));
        auto *price = new QLabel(I18n::instance().tr(QStringLiteral("vipPriceYuan")).arg(priceYuan, 0, 'f', 2), card);
        price->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: 700; color: %1;").arg(Theme::kLavender));
        top->addWidget(name);
        top->addStretch();
        top->addWidget(price);
        cardLay->addLayout(top);

        auto *payRow = new QHBoxLayout();
        payRow->setSpacing(10);

        auto *wxBtn = new QPushButton(I18n::instance().tr(QStringLiteral("vipPayWechat")), card);
        wxBtn->setFixedHeight(36);
        wxBtn->setCursor(Qt::PointingHandCursor);
        wxBtn->setStyleSheet(payBtnStyle(QString(Theme::kMint)));
        connect(wxBtn, &QPushButton::clicked, this, [this, pricingId]() { startPay(pricingId, QStringLiteral("wxpay")); });

        auto *aliBtn = new QPushButton(I18n::instance().tr(QStringLiteral("vipPayAlipay")), card);
        aliBtn->setFixedHeight(36);
        aliBtn->setCursor(Qt::PointingHandCursor);
        aliBtn->setStyleSheet(payBtnStyle(QString(Theme::kLavender)));
        connect(aliBtn, &QPushButton::clicked, this, [this, pricingId]() { startPay(pricingId, QStringLiteral("alipay")); });

        payRow->addWidget(wxBtn);
        payRow->addWidget(aliBtn);
        cardLay->addLayout(payRow);
        m_pricingLay->addWidget(card);
    }
}

QString VipPage::formatPlanLabel(const QVariantMap &item)
{
    const int months = item.value(QStringLiteral("months")).toInt();
    const int days = item.value(QStringLiteral("days")).toInt();
    if (months > 0)
        return QString::number(months) + I18n::instance().tr(QStringLiteral("vipMonthsSuffix"));
    if (days > 0)
        return QString::number(days) + I18n::instance().tr(QStringLiteral("vipDaysSuffix"));
    return I18n::instance().tr(QStringLiteral("vipPlanDefault"));
}

int VipPage::scorePayUrl(const QString &url, const QString &payType)
{
    const QUrl u(url);
    const QString scheme = u.scheme().toLower();
    const QString host = u.host().toLower();

    if (payType == QStringLiteral("wxpay")) {
        if (scheme == QStringLiteral("weixin") || scheme == QStringLiteral("wxp"))
            return 100;
        if (url.contains(QStringLiteral("h5.php"), Qt::CaseInsensitive))
            return 90;
        if (host.contains(QStringLiteral("z-pay")) || host.contains(QStringLiteral("mall.z-pay")))
            return 80;
        if (url.contains(QStringLiteral("wxpay"), Qt::CaseInsensitive))
            return 70;
        if (host.contains(QStringLiteral("tenpay")) || host.contains(QStringLiteral("weixin.qq.com")))
            return 60;
        return 10;
    }

    if (scheme == QStringLiteral("alipays") || scheme == QStringLiteral("alipay"))
        return 100;
    if (host == QStringLiteral("qr.alipay.com") || host.endsWith(QStringLiteral(".alipay.com")))
        return 85;
    if (url.contains(QStringLiteral("alipay"), Qt::CaseInsensitive))
        return 50;
    return 10;
}

QString VipPage::pickPayUrl(const QVariantMap &data, const QString &payType)
{
    QStringList urls;
    for (const char *key : {"payurl", "payurl2", "qrcode", "img"}) {
        const QString u = data.value(QString::fromUtf8(key)).toString().trimmed();
        if (!u.isEmpty() && !urls.contains(u))
            urls.append(u);
    }
    if (urls.isEmpty())
        return {};

    std::sort(urls.begin(), urls.end(), [&](const QString &a, const QString &b) {
        return scorePayUrl(a, payType) > scorePayUrl(b, payType);
    });
    return urls.first();
}

void VipPage::startPay(int pricingId, const QString &payType)
{
    if (!UserManager::instance().isLoggedIn()) {
        Toast::show(this, I18n::instance().tr(QStringLiteral("vipLoginRequired")));
        emit loginRequested();
        return;
    }
    if (!m_apiClient)
        return;

    m_apiClient->createVipPayOrder(pricingId, payType, [this, payType](bool ok, const QString &message, const QVariantMap &data) {
        if (!ok) {
            Toast::show(this, message.isEmpty()
                                ? I18n::instance().tr(QStringLiteral("vipPayCreateFailed"))
                                : message);
            return;
        }
        const QString url = pickPayUrl(data, payType);
        if (url.isEmpty()) {
            Toast::show(this, I18n::instance().tr(QStringLiteral("vipPayNoUrl")));
            return;
        }
        if (!QDesktopServices::openUrl(QUrl(url))) {
            Toast::show(this, I18n::instance().tr(QStringLiteral("vipPayOpenFailed")));
            return;
        }
        Toast::show(this, I18n::instance().tr(QStringLiteral("vipPayBrowserHint")));
    });
}
