#pragma once

/**
 * @file forgotpassworddialog.h
 * @brief 忘记密码对话框
 */

#include <QDialog>

class QLineEdit;
class QLabel;
class QPushButton;
class QStackedWidget;
class QWidget;
class ApiClient;

class ForgotPasswordDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ForgotPasswordDialog(QWidget *parent = nullptr);
    ~ForgotPasswordDialog() override;

private:
    void setupUi();
    void applyDialogTheme();
    void updateDialogSize();
    void setMsg(const QString &text, const QColor &color);
    void doSendResetCode();
    void doResetPassword();

    QWidget *m_card = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_emailHintLabel = nullptr;
    QLabel *m_codeHintLabel = nullptr;
    QStackedWidget *m_stack = nullptr;
    QLineEdit *m_emailEdit = nullptr;
    QLineEdit *m_codeEdit = nullptr;
    QLineEdit *m_newPassEdit = nullptr;
    QLineEdit *m_confirmPassEdit = nullptr;
    QPushButton *m_submitBtn = nullptr;
    QPushButton *m_sendCodeBtn = nullptr;
    QLabel *m_msgLabel = nullptr;
    ApiClient *m_api = nullptr;

    int m_countdown = 0;
};
