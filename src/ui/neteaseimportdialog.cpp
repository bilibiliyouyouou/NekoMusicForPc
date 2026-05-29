/**
 * @file neteaseimportdialog.cpp
 * @brief 网易云歌单导入对话框实现
 */

#include "neteaseimportdialog.h"
#include "core/apiclient.h"
#include "core/i18n.h"
#include "core/usermanager.h"
#include "theme/theme.h"
#include "theme/thememanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QProgressBar>
#include <QPainter>
#include <QRegularExpression>
#include <QGraphicsDropShadowEffect>

namespace {

void polishFramelessDialog(QDialog *dlg)
{
    dlg->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
    dlg->setAttribute(Qt::WA_TranslucentBackground);
    dlg->setModal(true);

    auto *shadow = new QGraphicsDropShadowEffect(dlg);
    shadow->setBlurRadius(26);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 100));
    dlg->setGraphicsEffect(shadow);
}

} // namespace

NeteaseImportDialog::NeteaseImportDialog(ApiClient *apiClient, QWidget *parent)
    : QDialog(parent)
    , m_apiClient(apiClient)
{
    polishFramelessDialog(this);
    setFixedSize(520, 480);
    setupUi();
}

void NeteaseImportDialog::setupUi()
{
    const bool dark = Theme::ThemeManager::instance().isDarkMode();
    
    // 颜色定义
    const QString bgColor = dark ? QStringLiteral("rgba(36, 36, 36, 245)") : QStringLiteral("rgba(255, 255, 255, 248)");
    const QString borderColor = dark ? QString::fromUtf8(Theme::kBorderGlass) : QStringLiteral("rgba(111, 66, 193, 0.22)");
    const QString textMainColor = dark ? QString::fromUtf8(Theme::kTextMain) : QStringLiteral("#212529");
    const QString textSubColor = dark ? QString::fromUtf8(Theme::kTextSub) : QStringLiteral("rgba(33, 37, 41, 0.65)");
    const QString inputBgColor = dark ? QStringLiteral("rgba(255, 255, 255, 0.08)") : QStringLiteral("rgba(33, 37, 41, 0.05)");
    const QString inputFocusBg = dark ? QStringLiteral("rgba(255, 255, 255, 0.12)") : QStringLiteral("#ffffff");
    const QString btnSecondaryBg = dark ? QStringLiteral("rgba(255, 255, 255, 0.1)") : QStringLiteral("rgba(33, 37, 41, 0.06)");
    
    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(20, 20, 20, 20);
    outerLayout->setSpacing(0);
    
    // 主容器（玻璃效果）
    auto *mainBox = new QWidget(this);
    mainBox->setObjectName(QStringLiteral("neteaseImportBox"));
    mainBox->setStyleSheet(
        QStringLiteral(
            "QWidget#neteaseImportBox {"
            "  background: %1;"
            "  border: 1px solid %2;"
            "  border-radius: %3px;"
            "}")
            .arg(bgColor, borderColor)
            .arg(Theme::kRMd));
    
    auto *mainLayout = new QVBoxLayout(mainBox);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    // 标题
    auto *titleLabel = new QLabel(I18n::instance().tr(QStringLiteral("importNeteasePlaylist")), mainBox);
    titleLabel->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: bold; color: %1; background: transparent;").arg(textMainColor));
    mainLayout->addWidget(titleLabel);

    // 说明文字
    auto *descLabel = new QLabel(I18n::instance().tr(QStringLiteral("importNeteaseDesc")), mainBox);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 13px; background: transparent;").arg(textSubColor));
    mainLayout->addWidget(descLabel);

    // 输入区域
    auto *inputLayout = new QHBoxLayout();
    inputLayout->setSpacing(8);
    
    m_inputEdit = new QLineEdit(mainBox);
    m_inputEdit->setPlaceholderText(I18n::instance().tr(QStringLiteral("inputNeteaseLink")));
    m_inputEdit->setStyleSheet(QStringLiteral(
        "QLineEdit { "
        "  background: %1; "
        "  border: 1px solid %2; "
        "  border-radius: 8px; "
        "  padding: 10px 14px; "
        "  color: %3; "
        "  font-size: 14px; "
        "}"
        "QLineEdit:focus { border-color: %4; background: %5; }"
    ).arg(inputBgColor, borderColor, textMainColor, QString::fromUtf8(Theme::kLavender), inputFocusBg));
    inputLayout->addWidget(m_inputEdit, 1);
    
    m_fetchBtn = new QPushButton(I18n::instance().tr(QStringLiteral("fetchPlaylist")), mainBox);
    m_fetchBtn->setCursor(Qt::PointingHandCursor);
    m_fetchBtn->setStyleSheet(QStringLiteral(
        "QPushButton { "
        "  background: %1; "
        "  color: white; "
        "  border: none; "
        "  border-radius: 8px; "
        "  padding: 10px 20px; "
        "  font-size: 14px; "
        "}"
        "QPushButton:hover { background: %2; }"
        "QPushButton:disabled { background: %3; color: %4; }"
    ).arg(QString::fromUtf8(Theme::kLavender), QString::fromUtf8(Theme::kLavenderLt),
          dark ? QStringLiteral("rgba(255,255,255,0.2)") : QStringLiteral("rgba(230, 57, 80, 0.45)"),
          dark ? QStringLiteral("rgba(255,255,255,0.4)") : QStringLiteral("rgba(33, 37, 41, 0.45)")));
    connect(m_fetchBtn, &QPushButton::clicked, this, &NeteaseImportDialog::onFetchPlaylist);
    inputLayout->addWidget(m_fetchBtn);
    
    mainLayout->addLayout(inputLayout);

    // 歌单信息显示
    m_playlistInfoLabel = new QLabel(mainBox);
    m_playlistInfoLabel->setWordWrap(true);
    m_playlistInfoLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 14px; padding: 8px 0; background: transparent;").arg(textMainColor));
    m_playlistInfoLabel->hide();
    mainLayout->addWidget(m_playlistInfoLabel);

    // 目标歌单选择
    auto *targetLayout = new QHBoxLayout();
    targetLayout->setSpacing(8);
    
    auto *targetLabel = new QLabel(I18n::instance().tr(QStringLiteral("selectTargetPlaylist")), mainBox);
    targetLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 14px; background: transparent;").arg(textMainColor));
    targetLayout->addWidget(targetLabel);
    
    m_targetPlaylistCombo = new QComboBox(mainBox);
    m_targetPlaylistCombo->setStyleSheet(QStringLiteral(
        "QComboBox { "
        "  background: %1; "
        "  border: 1px solid %2; "
        "  border-radius: 8px; "
        "  padding: 8px 12px; "
        "  color: %3; "
        "  font-size: 14px; "
        "  min-width: 200px; "
        "}"
        "QComboBox::drop-down { border: none; width: 24px; }"
        "QComboBox QAbstractItemView { "
        "  background: %4; "
        "  border: 1px solid %2; "
        "  border-radius: 8px; "
        "  color: %3; "
        "  selection-background-color: %5; "
        "}"
    ).arg(inputBgColor, borderColor, textMainColor, 
          dark ? QString::fromUtf8(Theme::kBgSurface) : QStringLiteral("#ffffff"),
          dark ? QStringLiteral("rgba(230, 57, 80, 0.38)") : QStringLiteral("rgba(230, 57, 80, 0.22)")));
    m_targetPlaylistCombo->hide();
    targetLayout->addWidget(m_targetPlaylistCombo, 1);
    
    mainLayout->addLayout(targetLayout);

    // 进度条
    m_progressBar = new QProgressBar(mainBox);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setStyleSheet(QStringLiteral(
        "QProgressBar { "
        "  background: %1; "
        "  border: none; "
        "  border-radius: 4px; "
        "  height: 8px; "
        "}"
        "QProgressBar::chunk { "
        "  background: %2; "
        "  border-radius: 4px; "
        "}"
    ).arg(dark ? QStringLiteral("rgba(255,255,255,0.1)") : QStringLiteral("rgba(33, 37, 41, 0.12)"),
          QString::fromUtf8(Theme::kLavender)));
    m_progressBar->hide();
    mainLayout->addWidget(m_progressBar);

    // 状态标签
    m_statusLabel = new QLabel(mainBox);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 13px; background: transparent;").arg(textSubColor));
    m_statusLabel->hide();
    mainLayout->addWidget(m_statusLabel);

    // 错误标签
    m_errorLabel = new QLabel(mainBox);
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setStyleSheet(QStringLiteral("color: #FF6B6B; font-size: 13px; background: transparent;"));
    m_errorLabel->hide();
    mainLayout->addWidget(m_errorLabel);

    mainLayout->addStretch();

    // 底部按钮
    auto *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);
    
    m_closeBtn = new QPushButton(I18n::instance().tr(QStringLiteral("close")), mainBox);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setStyleSheet(QStringLiteral(
        "QPushButton { "
        "  background: %1; "
        "  color: %2; "
        "  border: 1px solid %3; "
        "  border-radius: 8px; "
        "  padding: 10px 24px; "
        "  font-size: 14px; "
        "}"
        "QPushButton:hover { background: %4; }"
    ).arg(btnSecondaryBg, textMainColor, borderColor,
          dark ? QStringLiteral("rgba(255,255,255,0.15)") : QStringLiteral("rgba(230, 57, 80, 0.12)")));
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(m_closeBtn);
    
    btnLayout->addStretch();
    
    m_importBtn = new QPushButton(I18n::instance().tr(QStringLiteral("startImport")), mainBox);
    m_importBtn->setCursor(Qt::PointingHandCursor);
    m_importBtn->setStyleSheet(QStringLiteral(
        "QPushButton { "
        "  background: %1; "
        "  color: white; "
        "  border: none; "
        "  border-radius: 8px; "
        "  padding: 10px 24px; "
        "  font-size: 14px; "
        "}"
        "QPushButton:hover { background: %2; }"
        "QPushButton:disabled { background: %3; color: %4; }"
    ).arg(QString::fromUtf8(Theme::kLavender), QString::fromUtf8(Theme::kLavenderLt),
          dark ? QStringLiteral("rgba(255,255,255,0.2)") : QStringLiteral("rgba(230, 57, 80, 0.45)"),
          dark ? QStringLiteral("rgba(255,255,255,0.4)") : QStringLiteral("rgba(33, 37, 41, 0.45)")));
    m_importBtn->setEnabled(false);
    connect(m_importBtn, &QPushButton::clicked, this, &NeteaseImportDialog::onStartImport);
    btnLayout->addWidget(m_importBtn);
    
    mainLayout->addLayout(btnLayout);
    
    outerLayout->addWidget(mainBox);
}

QString NeteaseImportDialog::parsePlaylistId(const QString &input)
{
    const QString trimmed = input.trimmed();
    if (trimmed.isEmpty())
        return QString();
    
    // 纯数字
    static QRegularExpression digitsOnly(QStringLiteral("^\\d+$"));
    if (digitsOnly.match(trimmed).hasMatch())
        return trimmed;
    
    // URL 格式
    static QRegularExpression playlistQueryId(QStringLiteral("playlist\\?id=(\\d+)"), QRegularExpression::CaseInsensitiveOption);
    static QRegularExpression urlIdParam(QStringLiteral("[?&]id=(\\d+)"), QRegularExpression::CaseInsensitiveOption);
    static QRegularExpression playlistPathId(QStringLiteral("playlist/(\\d+)"), QRegularExpression::CaseInsensitiveOption);
    
    auto match = playlistQueryId.match(trimmed);
    if (match.hasMatch())
        return match.captured(1);
    
    match = urlIdParam.match(trimmed);
    if (match.hasMatch())
        return match.captured(1);
    
    match = playlistPathId.match(trimmed);
    if (match.hasMatch())
        return match.captured(1);
    
    return QString();
}

void NeteaseImportDialog::onFetchPlaylist()
{
    const QString input = m_inputEdit->text();
    const QString playlistIdStr = parsePlaylistId(input);
    
    if (playlistIdStr.isEmpty()) {
        setError(I18n::instance().tr(QStringLiteral("invalidNeteaseLink")));
        return;
    }
    
    bool ok = false;
    qint64 playlistId = playlistIdStr.toLongLong(&ok);
    if (!ok || playlistId <= 0) {
        setError(I18n::instance().tr(QStringLiteral("invalidNeteaseLink")));
        return;
    }
    
    setError(QString());
    m_fetchBtn->setEnabled(false);
    m_fetchBtn->setText(I18n::instance().tr(QStringLiteral("loading")));
    
    m_apiClient->fetchNeteasePlaylist(playlistId, [this, playlistId](bool success, const QString &message, const ApiClient::NeteasePlaylistInfo &playlist) {
        m_fetchBtn->setEnabled(true);
        m_fetchBtn->setText(I18n::instance().tr(QStringLiteral("fetchPlaylist")));
        
        if (!success) {
            setError(message.isEmpty() ? I18n::instance().tr(QStringLiteral("fetchPlaylistFailed")) : message);
            return;
        }
        
        if (playlist.tracks.isEmpty()) {
            setError(I18n::instance().tr(QStringLiteral("emptyNeteasePlaylist")));
            return;
        }
        
        // 保存信息
        m_neteasePlaylistId = playlist.id;
        m_neteasePlaylistName = playlist.name;
        m_neteaseTracks = playlist.tracks;
        
        // 显示歌单信息
        m_playlistInfoLabel->setText(I18n::instance().tr(QStringLiteral("neteasePlaylistInfo"))
            .arg(playlist.name)
            .arg(playlist.tracks.size()));
        m_playlistInfoLabel->show();
        
        // 加载用户歌单
        updatePlaylistCombo();
    });
}

void NeteaseImportDialog::updatePlaylistCombo()
{
    if (!UserManager::instance().isLoggedIn()) {
        setError(I18n::instance().tr(QStringLiteral("pleaseLoginFirst")));
        return;
    }
    
    m_apiClient->fetchUserPlaylists([this](bool success, const QList<QVariantMap> &playlists) {
        if (!success) {
            setError(I18n::instance().tr(QStringLiteral("loadPlaylistsFailed")));
            return;
        }
        
        m_userPlaylists.clear();
        m_targetPlaylistCombo->clear();
        
        for (const auto &pl : playlists) {
            int id = pl.value(QStringLiteral("id")).toInt();
            QString name = pl.value(QStringLiteral("name")).toString();
            m_userPlaylists.append(qMakePair(id, name));
            m_targetPlaylistCombo->addItem(name, id);
        }
        
        if (m_userPlaylists.isEmpty()) {
            setError(I18n::instance().tr(QStringLiteral("noPlaylistsToImport")));
            return;
        }
        
        m_targetPlaylistCombo->show();
        m_importBtn->setEnabled(true);
    });
}

void NeteaseImportDialog::onStartImport()
{
    if (m_neteaseTracks.isEmpty()) {
        setError(I18n::instance().tr(QStringLiteral("noTracksToImport")));
        return;
    }
    
    int targetPlaylistId = m_targetPlaylistCombo->currentData().toInt();
    if (targetPlaylistId <= 0) {
        setError(I18n::instance().tr(QStringLiteral("selectTargetPlaylist")));
        return;
    }
    
    // 禁用按钮
    m_fetchBtn->setEnabled(false);
    m_importBtn->setEnabled(false);
    m_targetPlaylistCombo->setEnabled(false);
    
    // 显示进度
    m_progressBar->show();
    m_progressBar->setValue(10);
    m_statusLabel->show();
    m_statusLabel->setText(I18n::instance().tr(QStringLiteral("searchingTracks")));
    setError(QString());
    
    // 转换为搜索项
    QList<ApiClient::BatchSearchItem> searchItems;
    for (const auto &track : m_neteaseTracks) {
        ApiClient::BatchSearchItem item;
        item.title = track.name;
        item.artist = track.artist;
        searchItems.append(item);
    }
    
    // 批量搜索
    m_apiClient->batchSearchMusic(searchItems, [this, targetPlaylistId](bool success, const ApiClient::BatchSearchResult &result) {
        if (!success) {
            setError(I18n::instance().tr(QStringLiteral("searchFailed")));
            m_fetchBtn->setEnabled(true);
            m_importBtn->setEnabled(true);
            m_targetPlaylistCombo->setEnabled(true);
            m_progressBar->hide();
            m_statusLabel->hide();
            return;
        }
        
        m_progressBar->setValue(50);
        
        if (result.matchedMusicIds.isEmpty()) {
            setError(I18n::instance().tr(QStringLiteral("noMatchedTracks")));
            m_fetchBtn->setEnabled(true);
            m_importBtn->setEnabled(true);
            m_targetPlaylistCombo->setEnabled(true);
            m_progressBar->hide();
            m_statusLabel->hide();
            return;
        }
        
        m_statusLabel->setText(I18n::instance().tr(QStringLiteral("addingToPlaylist"))
            .arg(result.matchedMusicIds.size()));
        
        // 批量添加
        m_apiClient->batchAddMusicToPlaylist(targetPlaylistId, result.matchedMusicIds,
            [this, searchResult = result](bool addSuccess, const ApiClient::BatchAddResult &addResult) {
                m_progressBar->setValue(100);
                
                if (!addSuccess) {
                    setError(I18n::instance().tr(QStringLiteral("addToPlaylistFailed")));
                    m_fetchBtn->setEnabled(true);
                    m_importBtn->setEnabled(true);
                    m_targetPlaylistCombo->setEnabled(true);
                    m_progressBar->hide();
                    m_statusLabel->hide();
                    return;
                }
                
                // 成功
                m_statusLabel->setText(I18n::instance().tr(QStringLiteral("importSuccess"))
                    .arg(addResult.addedCount)
                    .arg(m_neteaseTracks.size())
                    .arg(searchResult.failCount));
                m_statusLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 13px; background: transparent;").arg(QString::fromUtf8(Theme::kMint)));
                
                m_closeBtn->setText(I18n::instance().tr(QStringLiteral("close")));
                m_importBtn->hide();
                m_fetchBtn->hide();
                
                emit importCompleted();
            });
    });
}

void NeteaseImportDialog::setError(const QString &error)
{
    if (error.isEmpty()) {
        m_errorLabel->hide();
    } else {
        m_errorLabel->setText(error);
        m_errorLabel->show();
    }
}

void NeteaseImportDialog::setProgress(const QString &status)
{
    m_statusLabel->setText(status);
    m_statusLabel->show();
}

void NeteaseImportDialog::setStep(int step)
{
    m_currentStep = step;
}
