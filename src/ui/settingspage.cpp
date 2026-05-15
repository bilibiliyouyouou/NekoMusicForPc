/**
 * @file settingspage.cpp
 * @brief 设置页面实现
 */

#include "settingspage.h"
#include "core/i18n.h"
#include "theme/theme.h"
#include "theme/thememanager.h"
#include "ui/glasswidget.h"
#include "ui/scrollareafix.h"
#include "version.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QSettings>
#include <QDesktopServices>
#include <QUrl>

namespace {

constexpr auto kGithubUrl = "https://github.com/FantasyNetworkCN/NekoMusicForPc";
constexpr auto kApiDocsUrl = "https://github.com/FantasyNetworkCN/NekoMusicDocs";

} // namespace

SettingsPage::SettingsPage(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, false);
    setAutoFillBackground(false);
    setupUi();
}

void SettingsPage::setupUi()
{
    auto *scroll = new QScrollArea(this);
    scroll->setObjectName(QStringLiteral("settingsScroll"));
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setFrameShape(QFrame::NoFrame);

    auto *container = new QWidget(scroll);
    auto *lay = new QVBoxLayout(container);
    lay->setContentsMargins(32, 32, 32, 32);
    lay->setSpacing(16);

    auto *card = new GlassWidget(container);
    card->setBorderRadius(Theme::kRXl);
    card->setOpacity(0.58);
    QWidget *cardBody = card->contentWidget();

    auto *cardLay = new QVBoxLayout(cardBody);
    cardLay->setContentsMargins(24, 20, 24, 20);
    cardLay->setSpacing(16);

    // 语言设置
    auto *langRow = new QHBoxLayout();
    m_langLabel = new QLabel(I18n::instance().languageLabel(), cardBody);
    m_langLabel->setObjectName("settingsLabel");
    langRow->addWidget(m_langLabel);
    langRow->addStretch();

    m_langCombo = new QComboBox(cardBody);
    m_langCombo->setObjectName("settingsCombo");
    m_langCombo->blockSignals(true);
    m_langCombo->addItem(I18n::instance().languageChinese(), I18n::ZhCN);
    m_langCombo->addItem(I18n::instance().languageNya(), I18n::NyaCN);
    m_langCombo->addItem(I18n::instance().languageEnglish(), I18n::EnUS);

    // 恢复保存的语言设置
    QSettings settings;
    I18n::Language savedLang = static_cast<I18n::Language>(
        settings.value("language", static_cast<int>(I18n::ZhCN)).toInt());
    I18n::instance().setLanguage(savedLang);
    int idx = (savedLang == I18n::ZhCN) ? 0 : (savedLang == I18n::NyaCN) ? 1 : 2;
    m_langCombo->setCurrentIndex(idx);

    m_langCombo->blockSignals(false);
    connect(m_langCombo, QOverload<int>::of(&QComboBox::activated), this, [this](int index) {
        auto lang = static_cast<I18n::Language>(m_langCombo->itemData(index).toInt());
        I18n::instance().setLanguage(lang);
        // 保存设置
        QSettings settings;
        settings.setValue("language", static_cast<int>(lang));
        retranslate();
        emit languageChanged(lang);
    });
    langRow->addWidget(m_langCombo);
    cardLay->addLayout(langRow);

    // 分隔线
    auto *line = new QFrame(cardBody);
    line->setFrameShape(QFrame::HLine);
    line->setObjectName("settingsDivider");
    cardLay->addWidget(line);

    // 主题设置（桌面歌词入口在播放栏「词」按钮，此处不重复）
    auto *themeRow = new QHBoxLayout();
    auto *themeLabel = new QLabel(I18n::instance().tr("theme"), cardBody);
    themeLabel->setObjectName("settingsLabel");
    themeRow->addWidget(themeLabel);
    themeRow->addStretch();

    m_themeCombo = new QComboBox(cardBody);
    m_themeCombo->setObjectName("settingsCombo");
    m_themeCombo->blockSignals(true);
    m_themeCombo->addItem(I18n::instance().tr("themeSystem"), static_cast<int>(Theme::System));
    m_themeCombo->addItem(I18n::instance().tr("themeDark"), static_cast<int>(Theme::Dark));
    m_themeCombo->addItem(I18n::instance().tr("themeLight"), static_cast<int>(Theme::Light));

    const Theme::ThemeMode savedTheme = Theme::ThemeManager::instance().currentMode();
    const int themeIdx = (savedTheme == Theme::System) ? 0 : (savedTheme == Theme::Dark) ? 1 : 2;
    m_themeCombo->setCurrentIndex(themeIdx);

    m_themeCombo->blockSignals(false);
    connect(m_themeCombo, QOverload<int>::of(&QComboBox::activated), this, [this](int index) {
        const auto theme = static_cast<Theme::ThemeMode>(m_themeCombo->itemData(index).toInt());
        Theme::ThemeManager::instance().setMode(theme);
    });
    themeRow->addWidget(m_themeCombo);
    cardLay->addLayout(themeRow);

    auto *lineTheme = new QFrame(cardBody);
    lineTheme->setFrameShape(QFrame::HLine);
    lineTheme->setObjectName("settingsDivider");
    cardLay->addWidget(lineTheme);

    // 关于
    m_aboutLabel = new QLabel(I18n::instance().tr("about"), cardBody);
    m_aboutLabel->setObjectName("settingsLabel");
    cardLay->addWidget(m_aboutLabel);

    // 版本 & 系统
    m_versionLabel = new QLabel(QString("%1: %2").arg(I18n::instance().version(), QString::fromUtf8(APP_VERSION)), cardBody);
    m_versionLabel->setObjectName("settingsInfo");
    cardLay->addWidget(m_versionLabel);

    m_systemLabel = new QLabel(QString("%1: %2").arg(I18n::instance().system()).arg(QSysInfo::prettyProductName()), cardBody);
    m_systemLabel->setObjectName("settingsInfo");
    cardLay->addWidget(m_systemLabel);

    auto *linksRow = new QHBoxLayout();
    linksRow->setSpacing(20);

    m_githubBtn = new QPushButton(I18n::instance().tr("githubRepo"), cardBody);
    m_githubBtn->setObjectName("settingsLinkBtn");
    m_githubBtn->setCursor(Qt::PointingHandCursor);
    m_githubBtn->setFlat(true);
    connect(m_githubBtn, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl(QString::fromUtf8(kGithubUrl)));
    });
    linksRow->addWidget(m_githubBtn);

    m_apiDocsBtn = new QPushButton(I18n::instance().tr("apiDocs"), cardBody);
    m_apiDocsBtn->setObjectName("settingsLinkBtn");
    m_apiDocsBtn->setCursor(Qt::PointingHandCursor);
    m_apiDocsBtn->setFlat(true);
    connect(m_apiDocsBtn, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl(QString::fromUtf8(kApiDocsUrl)));
    });
    linksRow->addWidget(m_apiDocsBtn);
    linksRow->addStretch();
    cardLay->addLayout(linksRow);

    // 检查更新按钮
    m_checkUpdateBtn = new QPushButton(I18n::instance().tr("checkForUpdates"), cardBody);
    m_checkUpdateBtn->setObjectName("checkUpdateBtn");
    m_checkUpdateBtn->setFixedHeight(40);
    m_checkUpdateBtn->setCursor(Qt::PointingHandCursor);
    m_checkUpdateBtn->setStyleSheet(
        "QPushButton#checkUpdateBtn { "
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #667eea, stop:1 #764ba2); "
        "  color: white; "
        "  border: none; "
        "  border-radius: 8px; "
        "  font-size: 14px; "
        "  font-weight: 600; "
        "}"
        "QPushButton#checkUpdateBtn:hover { opacity: 0.9; }"
        "QPushButton#checkUpdateBtn:pressed { opacity: 0.8; }"
        "QPushButton#checkUpdateBtn:disabled { opacity: 0.6; }"
    );
    connect(m_checkUpdateBtn, &QPushButton::clicked, this, &SettingsPage::checkForUpdatesRequested);
    cardLay->addWidget(m_checkUpdateBtn);

    cardLay->addStretch();
    lay->addWidget(card);
    lay->addStretch();

    scroll->setWidget(container);
    nekoPolishScrollAreaViewport(scroll);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(scroll);
}

void SettingsPage::retranslate()
{
    m_langLabel->setText(I18n::instance().languageLabel());
    m_langCombo->setItemText(0, I18n::instance().languageChinese());
    m_langCombo->setItemText(1, I18n::instance().languageNya());
    m_langCombo->setItemText(2, I18n::instance().languageEnglish());
    if (m_themeCombo) {
        m_themeCombo->setItemText(0, I18n::instance().tr("themeSystem"));
        m_themeCombo->setItemText(1, I18n::instance().tr("themeDark"));
        m_themeCombo->setItemText(2, I18n::instance().tr("themeLight"));
    }
    if (m_aboutLabel)
        m_aboutLabel->setText(I18n::instance().tr("about"));
    m_versionLabel->setText(QString("%1: %2").arg(I18n::instance().version(), QString::fromUtf8(APP_VERSION)));
    m_systemLabel->setText(QString("%1: %2").arg(I18n::instance().system()).arg(QSysInfo::prettyProductName()));
    if (m_githubBtn)
        m_githubBtn->setText(I18n::instance().tr("githubRepo"));
    if (m_apiDocsBtn)
        m_apiDocsBtn->setText(I18n::instance().tr("apiDocs"));
    if (m_checkUpdateBtn)
        m_checkUpdateBtn->setText(I18n::instance().tr("checkForUpdates"));
}

void SettingsPage::paintEvent(QPaintEvent *) {}
