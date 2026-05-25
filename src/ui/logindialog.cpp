/**
 * @file logindialog.cpp
 * @brief 登录/注册对话框实现
 */

#include "logindialog.h"
#include "authdialogchrome.h"
#include "forgotpassworddialog.h"
#include "slidercaptchadialog.h"
#include "core/apiclient.h"
#include "core/usermanager.h"
#include "core/i18n.h"
#include "theme/theme.h"
#include "theme/thememanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QGraphicsDropShadowEffect>
#include <QTimer>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , m_api(new ApiClient(this))
{
    setStyleSheet(Theme::ThemeManager::instance().currentStyleSheet());
    setupUi();
    applyDialogTheme();

    setModal(true);
    setFixedWidth(AuthDialogChrome::kDialogWidth);
    updateDialogSize();
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(30);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 80));
    setGraphicsEffect(shadow);
}

LoginDialog::~LoginDialog() = default;

void LoginDialog::applyDialogTheme()
{
    const AuthDialogChrome::Palette p = AuthDialogChrome::currentPalette();

    if (m_card)
        m_card->setStyleSheet(AuthDialogChrome::cardStyleSheet(p));
    if (m_titleLabel)
        m_titleLabel->setStyleSheet(AuthDialogChrome::titleStyleSheet(p));
    if (m_msgLabel) {
        if (m_msgLabel->text().isEmpty()) {
            m_msgLabel->hide();
            m_msgLabel->setStyleSheet(AuthDialogChrome::msgStyleSheet(p.msgColor));
        } else {
            m_msgLabel->show();
        }
    }
}

void LoginDialog::updateDialogSize()
{
    const int minH = m_isLoginMode ? 420 : 540;
    adjustSize();
    const int h = qMax(minH, sizeHint().height());
    setMinimumHeight(minH);
    resize(AuthDialogChrome::kDialogWidth, h);
}

void LoginDialog::setupUi()
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

    auto *closeBtn = new QPushButton(QStringLiteral("×"), m_card);
    closeBtn->setObjectName("dialogCloseBtn");
    closeBtn->setFixedSize(34, 34);
    closeBtn->setCursor(Qt::PointingHandCursor);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);

    auto *headerRow = new QHBoxLayout();
    headerRow->setContentsMargins(0, 0, 0, 0);
    headerRow->addStretch();
    headerRow->addWidget(closeBtn);
    mainLayout->addLayout(headerRow);

    m_titleLabel = new QLabel(I18n::instance().tr("login"), m_card);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_titleLabel);

    m_msgLabel = new QLabel(m_card);
    m_msgLabel->setAlignment(Qt::AlignCenter);
    m_msgLabel->setWordWrap(true);
    m_msgLabel->hide();
    mainLayout->addWidget(m_msgLabel);

    auto *loginWidget = new QWidget(m_card);
    auto *loginLayout = new QVBoxLayout(loginWidget);
    loginLayout->setContentsMargins(0, 0, 0, 0);
    loginLayout->setSpacing(AuthDialogChrome::kFieldSpacing);

    m_loginUserEdit = new QLineEdit(loginWidget);
    m_loginUserEdit->setPlaceholderText(I18n::instance().tr("email"));
    m_loginUserEdit->setObjectName("dialogEdit");
    m_loginUserEdit->setFixedHeight(AuthDialogChrome::kFieldHeight);
    loginLayout->addWidget(m_loginUserEdit);

    m_loginPassEdit = new QLineEdit(loginWidget);
    m_loginPassEdit->setPlaceholderText(I18n::instance().tr("password"));
    m_loginPassEdit->setObjectName("dialogEdit");
    m_loginPassEdit->setFixedHeight(AuthDialogChrome::kFieldHeight);
    m_loginPassEdit->setEchoMode(QLineEdit::Password);
    loginLayout->addWidget(m_loginPassEdit);

    auto *regWidget = new QWidget(m_card);
    auto *regLayout = new QVBoxLayout(regWidget);
    regLayout->setContentsMargins(0, 0, 0, 0);
    regLayout->setSpacing(AuthDialogChrome::kFieldSpacing);

    m_regUserEdit = new QLineEdit(regWidget);
    m_regUserEdit->setPlaceholderText(I18n::instance().tr("username"));
    m_regUserEdit->setObjectName("dialogEdit");
    m_regUserEdit->setFixedHeight(AuthDialogChrome::kFieldHeight);
    regLayout->addWidget(m_regUserEdit);

    m_regPassEdit = new QLineEdit(regWidget);
    m_regPassEdit->setPlaceholderText(I18n::instance().tr("password"));
    m_regPassEdit->setObjectName("dialogEdit");
    m_regPassEdit->setFixedHeight(AuthDialogChrome::kFieldHeight);
    m_regPassEdit->setEchoMode(QLineEdit::Password);
    regLayout->addWidget(m_regPassEdit);

    auto *emailRow = new QWidget(regWidget);
    auto *emailRowLayout = new QHBoxLayout(emailRow);
    emailRowLayout->setContentsMargins(0, 0, 0, 0);
    emailRowLayout->setSpacing(10);

    m_regEmailEdit = new QLineEdit(emailRow);
    m_regEmailEdit->setPlaceholderText(I18n::instance().tr("email"));
    m_regEmailEdit->setObjectName("dialogEdit");
    m_regEmailEdit->setFixedHeight(AuthDialogChrome::kFieldHeight);
    emailRowLayout->addWidget(m_regEmailEdit, 1);

    m_sendCodeBtn = new QPushButton(I18n::instance().tr("sendCode"), emailRow);
    m_sendCodeBtn->setObjectName("dialogBtn");
    m_sendCodeBtn->setFixedHeight(AuthDialogChrome::kFieldHeight);
    m_sendCodeBtn->setMinimumWidth(108);
    connect(m_sendCodeBtn, &QPushButton::clicked, this, &LoginDialog::doSendVerificationCode);
    emailRowLayout->addWidget(m_sendCodeBtn);

    regLayout->addWidget(emailRow);

    m_regCodeEdit = new QLineEdit(regWidget);
    m_regCodeEdit->setPlaceholderText(I18n::instance().tr("verificationCode"));
    m_regCodeEdit->setObjectName("dialogEdit");
    m_regCodeEdit->setFixedHeight(AuthDialogChrome::kFieldHeight);
    regLayout->addWidget(m_regCodeEdit);

    m_stack = new QStackedWidget(m_card);
    m_stack->addWidget(loginWidget);
    m_stack->addWidget(regWidget);
    m_stack->setCurrentIndex(0);
    mainLayout->addWidget(m_stack);

    mainLayout->addSpacing(4);

    m_submitBtn = new QPushButton(I18n::instance().tr("login"), m_card);
    m_submitBtn->setObjectName("dialogBtn");
    m_submitBtn->setFixedHeight(AuthDialogChrome::kPrimaryBtnHeight);
    connect(m_submitBtn, &QPushButton::clicked, this, [this]() {
        if (m_isLoginMode)
            doLogin();
        else
            doRegister();
    });
    mainLayout->addWidget(m_submitBtn);

    auto *linksWrap = new QWidget(m_card);
    auto *linksLay = new QVBoxLayout(linksWrap);
    linksLay->setContentsMargins(0, 4, 0, 0);
    linksLay->setSpacing(10);

    m_switchBtn = new QPushButton(I18n::instance().tr("register"), linksWrap);
    m_switchBtn->setObjectName("dialogLinkBtn");
    m_switchBtn->setFixedHeight(AuthDialogChrome::kLinkBtnHeight);
    m_switchBtn->setCursor(Qt::PointingHandCursor);
    connect(m_switchBtn, &QPushButton::clicked, this, &LoginDialog::switchMode);
    linksLay->addWidget(m_switchBtn);

    m_forgotBtn = new QPushButton(I18n::instance().tr("forgotPassword"), linksWrap);
    m_forgotBtn->setObjectName("dialogLinkBtn");
    m_forgotBtn->setFixedHeight(AuthDialogChrome::kLinkBtnHeight);
    m_forgotBtn->setCursor(Qt::PointingHandCursor);
    connect(m_forgotBtn, &QPushButton::clicked, this, &LoginDialog::showForgotPassword);
    linksLay->addWidget(m_forgotBtn);
    mainLayout->addWidget(linksWrap);

    outer->addWidget(m_card);
}

void LoginDialog::switchMode()
{
    m_isLoginMode = !m_isLoginMode;
    m_msgLabel->clear();
    applyDialogTheme();

    if (m_isLoginMode) {
        m_stack->setCurrentIndex(0);
        m_submitBtn->setText(I18n::instance().tr("login"));
        m_switchBtn->setText(I18n::instance().tr("register"));
        m_forgotBtn->show();
        if (m_titleLabel)
            m_titleLabel->setText(I18n::instance().tr("login"));
    } else {
        m_stack->setCurrentIndex(1);
        m_submitBtn->setText(I18n::instance().tr("register"));
        m_switchBtn->setText(I18n::instance().tr("login"));
        m_forgotBtn->hide();
        if (m_titleLabel)
            m_titleLabel->setText(I18n::instance().tr("register"));
    }
    updateDialogSize();
}

void LoginDialog::doLogin()
{
    QString username = m_loginUserEdit->text().trimmed();
    QString password = m_loginPassEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        setMsg(I18n::instance().tr("fillUsernameAndPassword"), Theme::kSakura);
        return;
    }

    setMsg("", Qt::transparent);
    m_submitBtn->setEnabled(false);
    m_submitBtn->setText("...");

    m_api->login(username, password, [this](bool success, const QString &message,
                                             const QString &token, const QVariantMap &user) {
        QTimer::singleShot(0, this, [this, success, message, token, user]() {
            onLoginResult(success, message, token, user);
        });
    });
}

void LoginDialog::doRegister()
{
    QString username = m_regUserEdit->text().trimmed();
    QString password = m_regPassEdit->text();
    QString email = m_regEmailEdit->text().trimmed();
    QString code = m_regCodeEdit->text().trimmed();

    if (username.isEmpty() || password.isEmpty() || email.isEmpty() || code.isEmpty()) {
        setMsg(I18n::instance().tr("fillAllFields"), Theme::kSakura);
        return;
    }

    setMsg("", Qt::transparent);
    m_submitBtn->setEnabled(false);
    m_submitBtn->setText("...");

    m_api->registerUser(username, password, email, code,
                        [this](bool success, const QString &message,
                               const QString &token, const QVariantMap &user) {
        QTimer::singleShot(0, this, [this, success, message, token, user]() {
            onLoginResult(success, message, token, user);
        });
    });
}

void LoginDialog::doSendVerificationCode()
{
    QString email = m_regEmailEdit->text().trimmed();
    if (email.isEmpty()) {
        setMsg(I18n::instance().tr("pleaseEnterEmail"), Theme::kSakura);
        return;
    }
    const QString username = m_regUserEdit->text().trimmed();
    if (username.isEmpty()) {
        setMsg(I18n::instance().tr(QStringLiteral("registerNeedUsernameForCode")), Theme::kSakura);
        return;
    }

    m_sendCodeBtn->setEnabled(false);

    SliderCaptchaDialog captchaDlg(m_api, this);
    const int captchaResult = captchaDlg.exec();
    if (captchaResult != QDialog::Accepted) {
        m_sendCodeBtn->setEnabled(true);
        return;
    }
    const QString passToken = captchaDlg.captchaPassToken();
    if (passToken.isEmpty()) {
        m_sendCodeBtn->setEnabled(true);
        return;
    }

    m_api->sendVerificationCode(email, username, passToken, [this](bool success, const QString &message) {
        QTimer::singleShot(0, this, [this, success, message]() {
            if (success) {
                setMsg(message, Theme::kMint);
                if (m_countdownTimer) {
                    m_countdownTimer->stop();
                    m_countdownTimer->deleteLater();
                }
                m_countdown = 60;
                m_countdownTimer = new QTimer(this);
                connect(m_countdownTimer, &QTimer::timeout, this, [this]() {
                    m_countdown--;
                    m_sendCodeBtn->setText(QString("%1s").arg(m_countdown));
                    if (m_countdown <= 0) {
                        m_countdownTimer->stop();
                        m_countdownTimer->deleteLater();
                        m_countdownTimer = nullptr;
                        m_sendCodeBtn->setEnabled(true);
                        m_sendCodeBtn->setText(I18n::instance().tr("sendCode"));
                    }
                });
                m_countdownTimer->start(1000);
            } else {
                setMsg(message, Theme::kSakura);
                m_sendCodeBtn->setEnabled(true);
            }
        });
    });
}

void LoginDialog::onLoginResult(bool success, const QString &message,
                                 const QString &token, const QVariantMap &user)
{
    m_submitBtn->setEnabled(true);
    if (m_isLoginMode) {
        m_submitBtn->setText(I18n::instance().tr("login"));
    } else {
        m_submitBtn->setText(I18n::instance().tr("register"));
    }

    if (success) {
        UserManager::instance().setLoginInfo(token, user);
        accept();
    } else {
        setMsg(message, Theme::kSakura);
    }
}

void LoginDialog::showForgotPassword()
{
    ForgotPasswordDialog dlg(this);
    dlg.exec();
}

void LoginDialog::setMsg(const QString &text, const QColor &color)
{
    m_msgLabel->setText(text);
    if (text.isEmpty()) {
        m_msgLabel->hide();
        applyDialogTheme();
        updateDialogSize();
        return;
    }
    m_msgLabel->show();
    m_msgLabel->setStyleSheet(AuthDialogChrome::msgStyleSheet(color.name()));
    updateDialogSize();
}
