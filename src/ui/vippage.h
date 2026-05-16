#pragma once

/**
 * @file vippage.h
 * @brief 会员中心 — 状态展示、价目与浏览器收银台支付
 */

#include <QWidget>

class ApiClient;
class QLabel;
class QPushButton;
class QVBoxLayout;
class QScrollArea;
class QWidget;

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
    void rebuildPricingList(const QList<QVariantMap> &items);
    void updateHero();
    void startPay(int pricingId, const QString &payType);
    static QString pickPayUrl(const QVariantMap &data, const QString &payType);
    static QString formatPlanLabel(const QVariantMap &item);
    static int scorePayUrl(const QString &url, const QString &payType);

    ApiClient *m_apiClient = nullptr;
    QScrollArea *m_scroll = nullptr;
    QWidget *m_container = nullptr;
    QVBoxLayout *m_mainLay = nullptr;
    QLabel *m_titleLabel = nullptr;
    QPushButton *m_refreshBtn = nullptr;
    QWidget *m_heroCard = nullptr;
    QLabel *m_heroTitle = nullptr;
    QLabel *m_heroSubtitle = nullptr;
    QLabel *m_statusLabel = nullptr;
    QWidget *m_pricingHost = nullptr;
    QVBoxLayout *m_pricingLay = nullptr;
    bool m_loadingPricing = false;
};
