#include "shortcutcapturebutton.h"
#include "core/i18n.h"

#include <QFocusEvent>
#include <QKeyEvent>

ShortcutCaptureButton::ShortcutCaptureButton(QWidget *parent)
    : QPushButton(parent)
{
    setObjectName(QStringLiteral("shortcutCaptureBtn"));
    setFocusPolicy(Qt::StrongFocus);
    setFixedWidth(180);
    setFixedHeight(36);
    setCursor(Qt::PointingHandCursor);
    connect(this, &QPushButton::clicked, this, [this]() {
        if (m_capturing)
            setCapturing(false);
        else
            setCapturing(true);
    });
    updateLabel();
}

void ShortcutCaptureButton::setKeySequence(const QKeySequence &seq)
{
    if (m_sequence == seq)
        return;
    m_sequence = seq;
    updateLabel();
}

void ShortcutCaptureButton::setCapturing(bool on)
{
    if (m_capturing == on)
        return;
    m_capturing = on;
    if (m_capturing) {
        setFocus(Qt::OtherFocusReason);
        setStyleSheet(QStringLiteral(
            "QPushButton#shortcutCaptureBtn { border: 1px solid #667eea; border-radius: 6px; "
            "background: rgba(102,126,234,0.12); padding: 4px 10px; }"));
    } else {
        setStyleSheet(QString());
    }
    updateLabel();
}

void ShortcutCaptureButton::updateLabel()
{
    if (m_capturing) {
        setText(I18n::instance().tr(QStringLiteral("shortcutPressKeys")));
        return;
    }
    if (m_sequence.isEmpty())
        setText(I18n::instance().tr(QStringLiteral("shortcutNotSet")));
    else
        setText(m_sequence.toString(QKeySequence::NativeText));
}

void ShortcutCaptureButton::keyPressEvent(QKeyEvent *event)
{
    if (!m_capturing) {
        QPushButton::keyPressEvent(event);
        return;
    }

    event->accept();
    const int key = event->key();
    if (key == Qt::Key_Escape) {
        setCapturing(false);
        return;
    }
    if (key == Qt::Key_Backspace || key == Qt::Key_Delete) {
        setKeySequence({});
        setCapturing(false);
        emit keySequenceChanged(m_sequence);
        return;
    }
    if (key == Qt::Key_Control || key == Qt::Key_Shift || key == Qt::Key_Alt || key == Qt::Key_Meta)
        return;

    const QKeySequence seq(event->keyCombination());
    if (seq.isEmpty())
        return;

    setKeySequence(seq);
    setCapturing(false);
    emit keySequenceChanged(m_sequence);
}

void ShortcutCaptureButton::focusOutEvent(QFocusEvent *event)
{
    QPushButton::focusOutEvent(event);
    if (m_capturing)
        setCapturing(false);
}
