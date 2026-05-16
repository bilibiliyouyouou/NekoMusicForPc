#pragma once

/**
 * @file vippage.h
 * @brief 会员中心 — 左栏套餐 + 右栏扫码支付（对齐 Web UserVipView）
 */

#include <QWidget>
#include <QList>
#include <QVariantMap>

class ApiClient;
class QLabel;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class QScrollArea;
class QWidget;
class QNetworkAccessManager;
class QNetworkReply;

class VipPage : public QWidget
{
    Q_OBJECT

public:
    explicit VipPage(ApiClient *apiClient, QWidget *parent = nullptr);

    void refresh();
    void retranslate();

signals:
    void loginRequested();

private:
    void setupUi();
    void syncVipStatus();
    void loadPricing();
    void rebuildPlanStrip(const QList<QVariantMap> &items);
    void updateHero();
    void updateCheckoutPanel();
    void selectPlan(int pricingId);
    void clearPayQr();
    void startPay(const QString &payType);
    void showCheckoutQr(const QVariantMap &data, const QString &payLabel);
    void loadQrImageFromUrl(const QString &url, const QString &encodeFallback = QString());
    static QString formatPlanLabel(const QVariantMap &item);
    static QString firstNonEmpty(const QVariantMap &data, std::initializer_list<const char *> keys);
    const QVariantMap *selectedPlan() const;

    ApiClient *m_apiClient = nullptr;
    QNetworkAccessManager *m_nam = nullptr;
    QNetworkReply *m_qrImageReply = nullptr;

    QScrollArea *m_leftScroll = nullptr;
    QWidget *m_leftContainer = nullptr;
    QLabel *m_titleLabel = nullptr;
    QPushButton *m_refreshBtn = nullptr;
    QWidget *m_heroCard = nullptr;
    QLabel *m_heroTitle = nullptr;
    QLabel *m_heroSubtitle = nullptr;
    QLabel *m_statusLabel = nullptr;
    QWidget *m_planStripHost = nullptr;
    QHBoxLayout *m_planStripLay = nullptr;

    QWidget *m_checkoutAside = nullptr;
    QLabel *m_checkoutPlanLabel = nullptr;
    QLabel *m_checkoutPriceLabel = nullptr;
    QWidget *m_checkoutPayChoice = nullptr;
    QWidget *m_checkoutQrPanel = nullptr;
    QLabel *m_checkoutQrTitle = nullptr;
    QLabel *m_checkoutQrTip = nullptr;
    QLabel *m_checkoutQrImage = nullptr;
    QPushButton *m_checkoutBrowserBtn = nullptr;
    QPushButton *m_checkoutDoneBtn = nullptr;
    QPushButton *m_checkoutChangePayBtn = nullptr;
    QLabel *m_checkoutPlaceholder = nullptr;

    QList<QVariantMap> m_pricingRows;
    int m_selectedPlanId = -1;
    int m_payBusyPlanId = -1;
    bool m_loadingPricing = false;
    QString m_browserPayUrl;
};
