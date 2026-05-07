#pragma once

#include <QDialog>

class QLineEdit;
class QLabel;

/**
 * 程序内单行输入弹窗（毛玻璃卡片），替代 QInputDialog::getText。
 * 用于新建歌单、重命名、修改描述等。
 */
class LineInputDialog : public QDialog
{
    Q_OBJECT

public:
    /** @param confirmButtonText 主按钮文案；空则使用「确认」 */
    LineInputDialog(QWidget *parent,
                    const QString &title,
                    const QString &fieldLabel,
                    const QString &placeholder = QString(),
                    const QString &initialValue = QString(),
                    const QString &confirmButtonText = QString(),
                    bool allowEmptySubmit = false);

    /** 当前输入（已 trim） */
    QString value() const;

private:
    void setupUi();

    QString m_dialogTitle;
    QString m_fieldLabel;
    QString m_placeholder;
    QString m_initial;
    QString m_confirmText;
    bool m_allowEmpty = false;

    QLineEdit *m_lineEdit = nullptr;
    QLabel *m_errorLbl = nullptr;
};
