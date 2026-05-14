#pragma once

#include <QWidget>
#include <QList>

#include "core/musicinfo.h"

class QVBoxLayout;
class QScrollArea;
class QLabel;
class QPushButton;

class RecentPage : public QWidget
{
    Q_OBJECT

public:
    explicit RecentPage(QWidget *parent = nullptr);

    void retranslate();
    void refresh();

signals:
    void playRequested(const MusicInfo &info);
    void playAllRequested(const QList<MusicInfo> &results);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    void setupUi();
    void loadRecentPlays();

    QVBoxLayout *m_mainLay = nullptr;
    QScrollArea *m_scroll = nullptr;
    QWidget *m_container = nullptr;
    QLabel *m_titleLabel = nullptr;
    QPushButton *m_playAllBtn = nullptr;
    QVBoxLayout *m_listLay = nullptr;
    QList<MusicInfo> m_loadedRecent;
};
