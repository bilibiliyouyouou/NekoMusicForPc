/**
 * @file slidercaptchadialog.cpp
 */

#include "slidercaptchadialog.h"
#include "authdialogchrome.h"
#include "captchasliderrail.h"
#include "core/apiclient.h"
#include "core/i18n.h"
#include "theme/theme.h"
#include "theme/thememanager.h"

#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

SliderCaptchaDialog::SliderCaptchaDialog(ApiClient *api, QWidget *parent)
    : QDialog(parent)
    , m_api(api)
{
    setStyleSheet(Theme::ThemeManager::instance().currentStyleSheet());
    setupUi();
    applyDialogTheme();

    setModal(true);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(30);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 80));
    setGraphicsEffect(shadow);

    setFixedWidth(AuthDialogChrome::kDialogWidth);
    updateDialogSize();

    loadChallenge();
}

void SliderCaptchaDialog::setupUi()
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(AuthDialogChrome::kOuterPad, AuthDialogChrome::kOuterPad,
                              AuthDialogChrome::kOuterPad, AuthDialogChrome::kOuterPad);
    outer->setSpacing(0);

    m_card = new QWidget(this);
    m_card->setObjectName(QStringLiteral("authDialogCard"));
    auto *mainLayout = new QVBoxLayout(m_card);
    mainLayout->setContentsMargins(AuthDialogChrome::kCardPadH, AuthDialogChrome::kCardPadV,
                                   AuthDialogChrome::kCardPadH, AuthDialogChrome::kCardPadV);
    mainLayout->setSpacing(AuthDialogChrome::kSectionSpacing);

    auto *headerRow = new QHBoxLayout();
    headerRow->setContentsMargins(0, 0, 0, 0);
    headerRow->addStretch();
    m_closeBtn = new QPushButton(QStringLiteral("×"), m_card);
    m_closeBtn->setObjectName(QStringLiteral("dialogCloseBtn"));
    m_closeBtn->setFixedSize(34, 34);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    headerRow->addWidget(m_closeBtn);
    mainLayout->addLayout(headerRow);

    m_titleLabel = new QLabel(I18n::instance().tr(QStringLiteral("captchaSecurityTitle")), m_card);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_titleLabel);

    m_descLabel = new QLabel(I18n::instance().tr(QStringLiteral("captchaSecurityDesc")), m_card);
    m_descLabel->setWordWrap(true);
    m_descLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_descLabel);

    m_stage = new QWidget(m_card);
    m_stage->setFixedSize(m_bgW, m_bgH);
    m_bgLabel = new QLabel(m_stage);
    m_bgLabel->setScaledContents(false);
    m_bgLabel->setGeometry(0, 0, m_bgW, m_bgH);
    m_pieceLabel = new QLabel(m_stage);
    m_pieceLabel->setScaledContents(false);
    m_pieceLabel->raise();
    mainLayout->addWidget(m_stage, 0, Qt::AlignHCenter);

    m_rail = new CaptchaSliderRail(m_card);
    m_rail->setChallenge(m_bgW, m_sliderW);
    m_rail->setInteractive(false);
    connect(m_rail, &CaptchaSliderRail::offsetChanged, this, &SliderCaptchaDialog::syncPieceToSlider);
    connect(m_rail, &CaptchaSliderRail::interactionReleased, this, &SliderCaptchaDialog::onSliderReleased);
    mainLayout->addWidget(m_rail, 0, Qt::AlignHCenter);

    m_statusLabel = new QLabel(m_card);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setText(I18n::instance().tr(QStringLiteral("captchaLoading")));
    mainLayout->addWidget(m_statusLabel);

    auto *toolRow = new QHBoxLayout();
    toolRow->addStretch();
    m_refreshBtn = new QPushButton(I18n::instance().tr(QStringLiteral("captchaRefresh")), m_card);
    m_refreshBtn->setObjectName(QStringLiteral("dialogBtn"));
    m_refreshBtn->setFixedHeight(AuthDialogChrome::kFieldHeight);
    m_refreshBtn->setEnabled(false);
    connect(m_refreshBtn, &QPushButton::clicked, this, &SliderCaptchaDialog::loadChallenge);
    toolRow->addWidget(m_refreshBtn);
    toolRow->addStretch();
    mainLayout->addLayout(toolRow);

    outer->addWidget(m_card);
}

void SliderCaptchaDialog::applyDialogTheme()
{
    const AuthDialogChrome::Palette p = AuthDialogChrome::currentPalette();
    if (m_card)
        m_card->setStyleSheet(AuthDialogChrome::cardStyleSheet(p));
    if (m_titleLabel)
        m_titleLabel->setStyleSheet(AuthDialogChrome::titleStyleSheet(p));
    if (m_descLabel)
        m_descLabel->setStyleSheet(AuthDialogChrome::bodyStyleSheet(p));
    if (m_statusLabel) {
        m_statusLabel->setStyleSheet(AuthDialogChrome::msgStyleSheet(p.msgColor));
    }
}

void SliderCaptchaDialog::updateDialogSize()
{
    const int minH = 480;
    adjustSize();
    const int h = qMax(minH, sizeHint().height());
    setMinimumHeight(minH);
    resize(AuthDialogChrome::kDialogWidth, h);
}

bool SliderCaptchaDialog::decodeDataUrlToPixmap(const QString &dataUrl, QPixmap *out)
{
    if (!out)
        return false;
    const int comma = dataUrl.indexOf(QLatin1Char(','));
    if (comma < 0)
        return false;
    const QByteArray raw = QByteArray::fromBase64(dataUrl.mid(comma + 1).toUtf8());
    return out->loadFromData(raw, "PNG");
}

void SliderCaptchaDialog::setBusy(bool busy)
{
    m_refreshBtn->setEnabled(!busy && !m_captchaToken.isEmpty());
    m_rail->setVerifying(busy);
    m_rail->setInteractive(!busy && !m_captchaToken.isEmpty());
}

void SliderCaptchaDialog::syncPieceToSlider(int value)
{
    if (m_pieceLabel)
        m_pieceLabel->setGeometry(value, m_puzzleY, m_sliderW, m_sliderH);
}

void SliderCaptchaDialog::applyChallenge(const QVariantMap &data)
{
    m_captchaToken = data.value(QStringLiteral("captchaToken")).toString();
    m_bgW = data.value(QStringLiteral("bgWidth")).toInt();
    if (m_bgW <= 0)
        m_bgW = 300;
    m_bgH = data.value(QStringLiteral("bgHeight")).toInt();
    if (m_bgH <= 0)
        m_bgH = 180;
    m_puzzleY = data.value(QStringLiteral("puzzleY")).toInt();
    m_sliderW = data.value(QStringLiteral("sliderWidth")).toInt();
    if (m_sliderW <= 0)
        m_sliderW = 52;
    m_sliderH = data.value(QStringLiteral("sliderHeight")).toInt();
    if (m_sliderH <= 0)
        m_sliderH = 52;

    const QString bgUrl = data.value(QStringLiteral("bgImage")).toString();
    const QString pieceUrl = data.value(QStringLiteral("sliderImage")).toString();

    QPixmap bg;
    QPixmap piece;
    if (!decodeDataUrlToPixmap(bgUrl, &bg) || !decodeDataUrlToPixmap(pieceUrl, &piece)) {
        m_statusLabel->setText(I18n::instance().tr(QStringLiteral("captchaImageError")));
        m_captchaToken.clear();
        m_rail->setChallenge(m_bgW, m_sliderW);
        m_rail->setOffset(0);
        m_rail->setInteractive(false);
        m_refreshBtn->setEnabled(true);
        setBusy(false);
        updateDialogSize();
        return;
    }

    m_stage->setFixedSize(m_bgW, m_bgH);
    m_bgLabel->setGeometry(0, 0, m_bgW, m_bgH);
    m_bgLabel->setPixmap(bg);
    m_pieceLabel->setPixmap(piece);

    m_rail->setChallenge(m_bgW, m_sliderW);
    m_rail->blockSignals(true);
    m_rail->setOffset(0);
    m_rail->blockSignals(false);
    syncPieceToSlider(0);

    m_statusLabel->clear();
    setBusy(false);
    m_refreshBtn->setEnabled(true);
    updateDialogSize();
}

void SliderCaptchaDialog::loadChallenge()
{
    if (!m_api)
        return;

    m_captchaToken.clear();
    m_passToken.clear();
    m_verifying = false;
    m_rail->blockSignals(true);
    m_rail->setChallenge(m_bgW, m_sliderW);
    m_rail->setOffset(0);
    m_rail->blockSignals(false);
    m_rail->setVerifying(false);
    m_rail->setInteractive(false);
    m_refreshBtn->setEnabled(false);
    m_statusLabel->setText(I18n::instance().tr(QStringLiteral("captchaLoading")));
    applyDialogTheme();

    m_api->fetchSliderCaptchaChallenge([this](bool ok, const QString &message, const QVariantMap &data) {
        QTimer::singleShot(0, this, [this, ok, message, data]() {
            if (!ok || data.isEmpty()) {
                m_statusLabel->setText(message.isEmpty()
                                           ? I18n::instance().tr(QStringLiteral("captchaLoadFailed"))
                                           : message);
                m_refreshBtn->setEnabled(true);
                updateDialogSize();
                return;
            }
            applyChallenge(data);
        });
    });
}

void SliderCaptchaDialog::onSliderReleased()
{
    if (m_verifying || m_captchaToken.isEmpty() || !m_api)
        return;

    m_verifying = true;
    setBusy(true);
    m_statusLabel->setText(I18n::instance().tr(QStringLiteral("captchaVerifying")));
    applyDialogTheme();

    const int off = m_rail->offset();
    m_api->verifySliderCaptcha(m_captchaToken, off, [this](bool ok, const QString &message, const QString &pass) {
        QTimer::singleShot(0, this, [this, ok, message, pass]() {
            m_verifying = false;
            if (ok && !pass.isEmpty()) {
                m_passToken = pass;
                accept();
                return;
            }
            m_statusLabel->setText(message.isEmpty()
                                       ? I18n::instance().tr(QStringLiteral("captchaVerifyFail"))
                                       : message);
            loadChallenge();
        });
    });
}
