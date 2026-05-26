#pragma once

#include <QLabel>

/** 圆角封面（SPlayer .cover border-radius: 8px） */
class RoundCoverLabel : public QLabel
{
public:
    explicit RoundCoverLabel(int radius = 8, QWidget *parent = nullptr);

    void setPixmap(const QPixmap &pix);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_radius = 8;
};
