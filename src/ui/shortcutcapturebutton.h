#pragma once

#include <QPushButton>
#include <QKeySequence>

/** 点击后捕获按键组合，用于设置页快捷键编辑。 */
class ShortcutCaptureButton final : public QPushButton
{
    Q_OBJECT

public:
    explicit ShortcutCaptureButton(QWidget *parent = nullptr);

    QKeySequence keySequence() const { return m_sequence; }
    void setKeySequence(const QKeySequence &seq);
    bool isCapturing() const { return m_capturing; }

signals:
    void keySequenceChanged(const QKeySequence &seq);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    void updateLabel();
    void setCapturing(bool on);

    QKeySequence m_sequence;
    bool m_capturing = false;
};
