#pragma once

#include <QDialog>

class QCheckBox;

/**
 * @brief 程序内提示：是否将本应用设为默认音乐播放器（非系统原生 QMessageBox）
 */
class DefaultMusicPlayerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DefaultMusicPlayerDialog(QWidget *parent = nullptr);

    bool dontAskAgain() const;

    /** Windows：在 trySet 打开系统设置页后的程序内说明 */
    static void showWindowsDefaultAppsFollowUp(QWidget *parent);

private:
    QCheckBox *m_dontAskAgain = nullptr;
};
