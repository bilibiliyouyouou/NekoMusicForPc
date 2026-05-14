#pragma once

#include <QWidget>
#include <QList>

#include "core/musicinfo.h"

class QVBoxLayout;
class QHBoxLayout;
class QScrollArea;
class QLabel;
class QPushButton;
class ApiClient;

class FavoritesPage : public QWidget
{
    Q_OBJECT

public:
    explicit FavoritesPage(ApiClient *apiClient, QWidget *parent = nullptr);

    void retranslate();
    void refresh();

signals:
    void playRequested(int musicId, const QString& title, const QString& artist, const QString& coverUrl);
    void playAllRequested(const QList<MusicInfo> &results);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    void setupUi();
    void loadFavorites();

    ApiClient *m_apiClient = nullptr;
    QVBoxLayout *m_mainLay = nullptr;
    QScrollArea *m_scroll = nullptr;
    QWidget *m_container = nullptr;
    QLabel *m_titleLabel = nullptr;
    QPushButton *m_playAllBtn = nullptr;
    QVBoxLayout *m_listLay = nullptr;
    QLabel *m_statusLabel = nullptr;
    QList<MusicInfo> m_loadedFavorites;
};
