/**
 * @file forgotpassworddialog.cpp
 * @brief 忘记密码对话框实现
 */

#include "forgotpassworddialog.h"
#include "authdialogchrome.h"
#include "core/apiclient.h"
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

ForgotPasswordDialog::ForgotPasswordDialog(QWidget *parent)
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

ForgotPasswordDialog::~ForgotPasswordDialog() = default;

void ForgotPasswordDialog::applyDialogTheme()
{
    const AuthDialogChrome::Palette p = AuthDialogChrome::currentPalette();

    if (m_card)
        m_card->setStyleSheet(AuthDialogChrome::cardStyleSheet(p));
    if (m_titleLabel)
        m_titleLabel->setStyleSheet(AuthDialogChrome::titleStyleSheet(p));
    if (m_emailHintLabel)
        m_emailHintLabel->setStyleSheet(AuthDialogChrome::bodyStyleSheet(p));
    if (m_codeHintLabel)
        m_codeHintLabel->setStyleSheet(AuthDialogChrome::bodyStyleSheet(p));
    if (m_msgLabel) {
        if (m_msgLabel->text().isEmpty()) {
            m_msgLabel->hide();
            m_msgLabel->setStyleSheet(AuthDialogChrome::msgStyleSheet(p.msgColor));
        } else {
            m_msgLabel->show();
        }
    }
}

void ForgotPasswordDialog::updateDialogSize()
{
    const int minH = m_stack && m_stack->currentIndex() == 1 ? 500 : 400;
    adjustSize();
    const int h = qMax(minH, sizeHint().height());
    setMinimumHeight(minH);
    resize(AuthDialogChrome::kDialogWidth, h);
}

void ForgotPasswordDialog::setupUi()
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

    m_titleLabel = new QLabel(QStringLiteral("找回密码"), m_card);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_titleLabel);

    m_msgLabel = new QLabel(m_card);
    m_msgLabel->setAlignment(Qt::AlignCenter);
    m_msgLabel->setWordWrap(true);
    m_msgLabel->hide();
    mainLayout->addWidget(m_msgLabel);

    m_stack = new QStackedWidget(m_card);

    auto *step1Widget = new QWidget(m_card);
    auto *step1Layout = new QVBoxLayout(step1Widget);
    step1Layout->setContentsMargins(0, 0, 0, 0);
    step1Layout->setSpacing(AuthDialogChrome::kFieldSpacing);

    m_emailHintLabel = new QLabel(QStringLiteral("请输入注册时的邮箱地址"), step1Widget);
    m_emailHintLabel->setWordWrap(true);
    step1Layout->addWidget(m_emailHintLabel);
    step1Layout->addSpacing(4);

    m_emailEdit = new QLineEdit(step1Widget);
    m_emailEdit->setPlaceholderText(I18n::instance().tr("email"));
    m_emailEdit->setObjectName("dialogEdit");
    m_emailEdit->setFixedHeight(AuthDialogChrome::kFieldHeight);
    step1Layout->addWidget(m_emailEdit);

    step1Layout->addSpacing(6);

    m_sendCodeBtn = new QPushButton(I18n::instance().tr("sendCode"), step1Widget);
    m_sendCodeBtn->setObjectName("dialogBtn");
    m_sendCodeBtn->setFixedHeight(AuthDialogChrome::kPrimaryBtnHeight);
    connect(m_sendCodeBtn, &QPushButton::clicked, this, &ForgotPasswordDialog::doSendResetCode);
    step1Layout->addWidget(m_sendCodeBtn);

    m_stack->addWidget(step1Widget);

    auto *step2Widget = new QWidget(m_card);
    auto *step2Layout = new QVBoxLayout(step2Widget);
    step2Layout->setContentsMargins(0, 0, 0, 0);
    step2Layout->setSpacing(AuthDialogChrome::kFieldSpacing);

    m_codeHintLabel = new QLabel(QStringLiteral("请输入邮箱中的验证码"), step2Widget);
    m_codeHintLabel->setWordWrap(true);
    step2Layout->addWidget(m_codeHintLabel);
    step2Layout->addSpacing(4);

    m_codeEdit = new QLineEdit(step2Widget);
    m_codeEdit->setPlaceholderText(I18n::instance().tr("verificationCode"));
    m_codeEdit->setObjectName("dialogEdit");
    m_codeEdit->setFixedHeight(AuthDialogChrome::kFieldHeight);
    step2Layout->addWidget(m_codeEdit);

    m_newPassEdit = new QLineEdit(step2Widget);
    m_newPassEdit->setPlaceholderText(QStringLiteral("新密码(6-30位)"));
    m_newPassEdit->setObjectName("dialogEdit");
    m_newPassEdit->setFixedHeight(AuthDialogChrome::kFieldHeight);
    m_newPassEdit->setEchoMode(QLineEdit::Password);
    step2Layout->addWidget(m_newPassEdit);

    m_confirmPassEdit = new QLineEdit(step2Widget);
    m_confirmPassEdit->setPlaceholderText(QStringLiteral("确认新密码"));
    m_confirmPassEdit->setObjectName("dialogEdit");
    m_confirmPassEdit->setFixedHeight(AuthDialogChrome::kFieldHeight);
    m_confirmPassEdit->setEchoMode(QLineEdit::Password);
    step2Layout->addWidget(m_confirmPassEdit);

    step2Layout->addSpacing(6);

    m_submitBtn = new QPushButton(QStringLiteral("重置密码"), step2Widget);
    m_submitBtn->setObjectName("dialogBtn");
    m_submitBtn->setFixedHeight(AuthDialogChrome::kPrimaryBtnHeight);
    connect(m_submitBtn, &QPushButton::clicked, this, &ForgotPasswordDialog::doResetPassword);
    step2Layout->addWidget(m_submitBtn);

    m_stack->addWidget(step2Widget);

    m_stack->setCurrentIndex(0);
    mainLayout->addWidget(m_stack);

    auto *backBtn = new QPushButton(QStringLiteral("返回登录"), m_card);
    backBtn->setObjectName("dialogLinkBtn");
    backBtn->setFixedHeight(AuthDialogChrome::kLinkBtnHeight);
    backBtn->setCursor(Qt::PointingHandCursor);
    connect(backBtn, &QPushButton::clicked, this, &QDialog::reject);
    mainLayout->addWidget(backBtn);

    outer->addWidget(m_card);
}

void ForgotPasswordDialog::setMsg(const QString &text, const QColor &color)
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

void ForgotPasswordDialog::doSendResetCode()
{
    QString email = m_emailEdit->text().trimmed();
    if (email.isEmpty()) {
        setMsg(I18n::instance().tr("pleaseEnterEmail"), Theme::kSakura);
        return;
    }

    setMsg("", Qt::transparent);
    m_sendCodeBtn->setEnabled(false);

    m_api->sendResetCode(email, [this](bool success, const QString &message) {
        QTimer::singleShot(0, this, [this, success, message]() {
            if (success) {
                setMsg(message, Theme::kMint);
                m_stack->setCurrentIndex(1);
                updateDialogSize();
                m_countdown = 60;
                auto *timer = new QTimer(this);
                connect(timer, &QTimer::timeout, this, [this, timer]() {
                    m_countdown--;
                    if (m_countdown > 0) {
                        m_sendCodeBtn->setText(QString("%1s").arg(m_countdown));
                    } else {
                        timer->stop();
                        timer->deleteLater();
                        m_sendCodeBtn->setEnabled(true);
                        m_sendCodeBtn->setText(I18n::instance().tr("sendCode"));
                    }
                });
                timer->start(1000);
            } else {
                setMsg(message, Theme::kSakura);
                m_sendCodeBtn->setEnabled(true);
            }
        });
    });
}

void ForgotPasswordDialog::doResetPassword()
{
    QString email = m_emailEdit->text().trimmed();
    QString code = m_codeEdit->text().trimmed();
    QString newPass = m_newPassEdit->text();
    QString confirmPass = m_confirmPassEdit->text();

    if (email.isEmpty() || code.isEmpty() || newPass.isEmpty() || confirmPass.isEmpty()) {
        setMsg(I18n::instance().tr("fillAllFields"), Theme::kSakura);
        return;
    }

    if (newPass.length() < 6 || newPass.length() > 30) {
        setMsg(QStringLiteral("密码长度必须在6-30位之间"), Theme::kSakura);
        return;
    }

    if (newPass != confirmPass) {
        setMsg(QStringLiteral("两次输入的密码不一致"), Theme::kSakura);
        return;
    }

    setMsg("", Qt::transparent);
    m_submitBtn->setEnabled(false);
    m_submitBtn->setText("...");

    m_api->resetPassword(email, code, newPass, [this](bool success, const QString &message) {
        QTimer::singleShot(0, this, [this, success, message]() {
            m_submitBtn->setEnabled(true);
            m_submitBtn->setText(QStringLiteral("重置密码"));

            if (success) {
                setMsg(QStringLiteral("密码重置成功,请登录"), Theme::kMint);
                QTimer::singleShot(1500, this, &QDialog::accept);
            } else {
                setMsg(message, Theme::kSakura);
            }
        });
    });
}
