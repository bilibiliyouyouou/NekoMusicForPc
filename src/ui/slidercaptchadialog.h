#pragma once

/**
 * @file slidercaptchadialog.h
 * @brief 注册发码前：滑块拼图人机验证（与 Web/Android 同一套后端接口）
 */

#include <QDialog>

class ApiClient;
class CaptchaSliderRail;
class QLabel;
class QPushButton;
class QWidget;

class SliderCaptchaDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit SliderCaptchaDialog(ApiClient *api, QWidget *parent = nullptr);

    QString captchaPassToken() const { return m_passToken; }

private:
    void setupUi();
    void applyDialogTheme();
    void updateDialogSize();
    void loadChallenge();
    void applyChallenge(const QVariantMap &data);
    void setBusy(bool busy);
    void syncPieceToSlider(int value);
    void onSliderReleased();
    static bool decodeDataUrlToPixmap(const QString &dataUrl, QPixmap *out);

    ApiClient *m_api = nullptr;
    QString m_captchaToken;
    QString m_passToken;
    int m_bgW = 300;
    int m_bgH = 180;
    int m_puzzleY = 0;
    int m_sliderW = 52;
    int m_sliderH = 52;
    bool m_verifying = false;

    QWidget *m_card = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_descLabel = nullptr;
    QWidget *m_stage = nullptr;
    QLabel *m_bgLabel = nullptr;
    QLabel *m_pieceLabel = nullptr;
    CaptchaSliderRail *m_rail = nullptr;
    QLabel *m_statusLabel = nullptr;
    QPushButton *m_refreshBtn = nullptr;
    QPushButton *m_closeBtn = nullptr;
};
