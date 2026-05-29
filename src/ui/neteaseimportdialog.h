#pragma once

/**
 * @file neteaseimportdialog.h
 * @brief 网易云歌单导入对话框
 */

#include <QDialog>
#include <QList>
#include <QPair>
#include "core/apiclient.h"

class QLineEdit;
class QLabel;
class QPushButton;
class QComboBox;
class QProgressBar;

/**
 * 网易云歌单导入对话框
 * 支持输入网易云歌单链接或ID，导入到本地歌单
 */
class NeteaseImportDialog : public QDialog
{
    Q_OBJECT

public:
    NeteaseImportDialog(ApiClient *apiClient, QWidget *parent = nullptr);

signals:
    void importCompleted(int addedCount, int totalCount, int failCount, bool importedToFavorites);

private slots:
    void onFetchPlaylist();
    void onStartImport();

private:
    void setupUi();
    void updatePlaylistCombo();
    void doImport(int targetPlaylistId);
    void addMatchedTracks(int targetPlaylistId, const ApiClient::BatchSearchResult &searchResult);
    void restoreImportControls();
    void finishImport(const ApiClient::BatchSearchResult &searchResult,
                      bool addSuccess,
                      const ApiClient::BatchAddResult &addResult,
                      bool importedToFavorites);
    void setStep(int step);
    void setError(const QString &error);
    void setProgress(const QString &status);
    
    static QString parsePlaylistId(const QString &input);

    static constexpr int kImportTargetFavorites = -2;
    static constexpr int kImportTargetNewPlaylist = -1;

    ApiClient *m_apiClient = nullptr;
    
    // UI 组件
    QLineEdit *m_inputEdit = nullptr;
    QPushButton *m_fetchBtn = nullptr;
    QLabel *m_playlistInfoLabel = nullptr;
    QComboBox *m_targetPlaylistCombo = nullptr;
    QLineEdit *m_newPlaylistEdit = nullptr;
    QPushButton *m_importBtn = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_errorLabel = nullptr;
    QPushButton *m_closeBtn = nullptr;
    
    // 状态
    int m_currentStep = 1;  // 1=输入链接, 2=选择歌单, 3=导入中, 4=完成
    qint64 m_neteasePlaylistId = 0;
    QString m_neteasePlaylistName;
    QList<ApiClient::NeteaseTrack> m_neteaseTracks;
    QList<QPair<int, QString>> m_userPlaylists;  // id, name
};
