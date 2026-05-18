#pragma once

/**
 * @file logindialog.h
 * @brief 登录/注册对话框
 */

#include <QDialog>

class QLineEdit;
class QLabel;
class QPushButton;
class QStackedWidget;
class QWidget;
class ApiClient;
class QTimer;

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog() override;

private:
    void setupUi();
    void applyDialogTheme();
    void updateDialogSize();
    void switchMode();
    void doLogin();
    void doRegister();
    void doSendVerificationCode();
    void onLoginResult(bool success, const QString &message,
                       const QString &token, const QVariantMap &user);
    void showForgotPassword();
    void setMsg(const QString &text, const QColor &color);

    QWidget *m_card = nullptr;
    QLabel *m_titleLabel = nullptr;
    QTimer *m_countdownTimer = nullptr;
    QStackedWidget *m_stack = nullptr;
    QLineEdit *m_loginUserEdit = nullptr;
    QLineEdit *m_loginPassEdit = nullptr;
    QLineEdit *m_regUserEdit = nullptr;
    QLineEdit *m_regPassEdit = nullptr;
    QLineEdit *m_regEmailEdit = nullptr;
    QLineEdit *m_regCodeEdit = nullptr;
    QPushButton *m_submitBtn = nullptr;
    QPushButton *m_switchBtn = nullptr;
    QPushButton *m_sendCodeBtn = nullptr;
    QPushButton *m_forgotBtn = nullptr;
    QLabel *m_msgLabel = nullptr;
    ApiClient *m_api = nullptr;
    bool m_isLoginMode = true;
    int m_countdown = 0;
};
