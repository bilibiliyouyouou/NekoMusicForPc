/**
 * @file settingspage.cpp
 * @brief 设置页面实现
 */

#include "settingspage.h"
#include "core/i18n.h"
#include "core/appshortcuts.h"
#include "core/globalshortcutcontroller.h"
#include "core/shellbackdropsettings.h"
#include "ui/shortcutcapturebutton.h"
#include "ui/toast.h"
#include "theme/theme.h"
#include "theme/thememanager.h"
#include "ui/glasswidget.h"
#include "ui/glasspaint.h"
#include "ui/scrollareafix.h"
#include "ui/svgicon.h"
#include "version.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QFrame>
#include <QSettings>
#include <QDesktopServices>
#include <QUrl>
#include <QFileDialog>
#include <QColorDialog>
#include <QFileInfo>
#include <QPixmap>
#include <QColor>

namespace {

constexpr auto kGithubUrl = "https://github.com/FantasyNetworkCN/NekoMusicForPc";
constexpr auto kApiDocsUrl = "https://github.com/FantasyNetworkCN/NekoMusicDocs";

} // namespace

SettingsPage::SettingsPage(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, false);
    setAutoFillBackground(false);
    setupUi();
    connect(&Theme::ThemeManager::instance(), &Theme::ThemeManager::themeChanged, this,
            [this](Theme::ThemeMode mode) {
                Q_UNUSED(mode);
                const auto cards = findChildren<GlassWidget *>();
                for (auto *card : cards)
                    GlassPaint::applyFlatSurface(card, Theme::ThemeManager::instance().isDarkMode());
            });
}

QWidget *SettingsPage::createSettingsCard(QWidget *parent, QVBoxLayout **layoutOut)
{
    auto *card = new GlassWidget(parent);
    card->setBorderRadius(Theme::kRXl);
    GlassPaint::applyFlatSurface(card, Theme::ThemeManager::instance().isDarkMode());
    QWidget *cardBody = card->contentWidget();

    auto *cardLay = new QVBoxLayout(cardBody);
    cardLay->setContentsMargins(24, 20, 24, 20);
    cardLay->setSpacing(16);
    if (layoutOut)
        *layoutOut = cardLay;
    return card;
}

QPushButton *SettingsPage::createTabButton(const QString &text, const char *iconName, QWidget *parent)
{
    auto *btn = new QPushButton(text, parent);
    btn->setObjectName(QStringLiteral("settingsTabBtn"));
    btn->setCheckable(true);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setMinimumHeight(38);
    btn->setIcon(Icons::renderNamed(iconName, 18, QColor(230, 57, 80)));
    btn->setIconSize(QSize(18, 18));
    return btn;
}

void SettingsPage::setActiveSettingsTab(int index)
{
    if (!m_settingsStack || index < 0 || index >= m_settingsStack->count())
        return;
    m_settingsStack->setCurrentIndex(index);
    const QList<QPushButton *> tabs = {m_generalTabBtn, m_appearanceTabBtn, m_shortcutsTabBtn, m_aboutTabBtn};
    for (int i = 0; i < tabs.size(); ++i) {
        if (tabs[i])
            tabs[i]->setChecked(i == index);
    }
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

    auto *headerRow = new QWidget(container);
    auto *headerLay = new QHBoxLayout(headerRow);
    headerLay->setContentsMargins(0, 0, 0, 0);
    headerLay->setSpacing(12);

    auto *titleWrap = new QWidget(headerRow);
    auto *titleLay = new QVBoxLayout(titleWrap);
    titleLay->setContentsMargins(0, 0, 0, 0);
    titleLay->setSpacing(4);
    m_titleLabel = new QLabel(I18n::instance().settingsTitle(), titleWrap);
    m_titleLabel->setObjectName(QStringLiteral("settingsTitle"));
    titleLay->addWidget(m_titleLabel);
    m_descLabel = new QLabel(I18n::instance().tr(QStringLiteral("settingsDesc")), titleWrap);
    m_descLabel->setObjectName(QStringLiteral("settingsInfo"));
    m_descLabel->setWordWrap(true);
    titleLay->addWidget(m_descLabel);
    headerLay->addWidget(titleWrap, 1);
    lay->addWidget(headerRow);

    auto *tabsRow = new QWidget(container);
    tabsRow->setObjectName(QStringLiteral("settingsTabBar"));
    auto *tabsLay = new QHBoxLayout(tabsRow);
    tabsLay->setContentsMargins(0, 0, 0, 0);
    tabsLay->setSpacing(8);
    m_generalTabBtn = createTabButton(I18n::instance().tr(QStringLiteral("settingsTabGeneral")), "Settings", tabsRow);
    m_appearanceTabBtn = createTabButton(I18n::instance().tr(QStringLiteral("settingsTabAppearance")), "Palette", tabsRow);
    m_shortcutsTabBtn = createTabButton(I18n::instance().tr(QStringLiteral("settingsTabShortcuts")), "Keyboard", tabsRow);
    m_aboutTabBtn = createTabButton(I18n::instance().tr(QStringLiteral("settingsTabAbout")), "Info", tabsRow);
    tabsLay->addWidget(m_generalTabBtn);
    tabsLay->addWidget(m_appearanceTabBtn);
    tabsLay->addWidget(m_shortcutsTabBtn);
    tabsLay->addWidget(m_aboutTabBtn);
    tabsLay->addStretch();
    lay->addWidget(tabsRow);

    m_settingsStack = new QStackedWidget(container);
    m_settingsStack->setObjectName(QStringLiteral("settingsStack"));
    lay->addWidget(m_settingsStack);

    connect(m_generalTabBtn, &QPushButton::clicked, this, [this]() { setActiveSettingsTab(0); });
    connect(m_appearanceTabBtn, &QPushButton::clicked, this, [this]() { setActiveSettingsTab(1); });
    connect(m_shortcutsTabBtn, &QPushButton::clicked, this, [this]() { setActiveSettingsTab(2); });
    connect(m_aboutTabBtn, &QPushButton::clicked, this, [this]() { setActiveSettingsTab(3); });

    QVBoxLayout *generalLay = nullptr;
    QWidget *generalCard = createSettingsCard(container, &generalLay);
    QWidget *generalBody = static_cast<GlassWidget *>(generalCard)->contentWidget();

    // 语言设置
    auto *langRow = new QHBoxLayout();
    m_langLabel = new QLabel(I18n::instance().languageLabel(), generalBody);
    m_langLabel->setObjectName("settingsLabel");
    langRow->addWidget(m_langLabel);
    langRow->addStretch();

    m_langCombo = new QComboBox(generalBody);
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
    generalLay->addLayout(langRow);
    generalLay->addStretch();

    QVBoxLayout *appearanceLay = nullptr;
    QWidget *appearanceCard = createSettingsCard(container, &appearanceLay);
    QWidget *appearanceBody = static_cast<GlassWidget *>(appearanceCard)->contentWidget();

    // 主题设置（桌面歌词入口在播放栏「词」按钮，此处不重复）
    auto *themeRow = new QHBoxLayout();
    m_themeLabel = new QLabel(I18n::instance().tr("theme"), appearanceBody);
    m_themeLabel->setObjectName("settingsLabel");
    themeRow->addWidget(m_themeLabel);
    themeRow->addStretch();

    m_themeCombo = new QComboBox(appearanceBody);
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
    appearanceLay->addLayout(themeRow);

    auto *linePersonalize = new QFrame(appearanceBody);
    linePersonalize->setFrameShape(QFrame::HLine);
    linePersonalize->setObjectName("settingsDivider");
    appearanceLay->addWidget(linePersonalize);

    setupPersonalizationSection(appearanceLay, appearanceBody);
    appearanceLay->addStretch();

    QVBoxLayout *shortcutsLay = nullptr;
    QWidget *shortcutsCard = createSettingsCard(container, &shortcutsLay);
    QWidget *shortcutsBody = static_cast<GlassWidget *>(shortcutsCard)->contentWidget();

    m_shortcutsSectionLabel = new QLabel(I18n::instance().tr("shortcuts"), shortcutsBody);
    m_shortcutsSectionLabel->setObjectName("settingsLabel");
    shortcutsLay->addWidget(m_shortcutsSectionLabel);

    setupShortcutRow(shortcutsLay, shortcutsBody, AppShortcuts::PlayPause, &m_shortcutPlayPauseLabel,
                     &m_shortcutPlayPauseBtn, &m_shortcutResetPlayPauseBtn);
    setupShortcutRow(shortcutsLay, shortcutsBody, AppShortcuts::PreviousTrack, &m_shortcutPrevLabel,
                     &m_shortcutPrevBtn, &m_shortcutResetPrevBtn);
    setupShortcutRow(shortcutsLay, shortcutsBody, AppShortcuts::NextTrack, &m_shortcutNextLabel,
                     &m_shortcutNextBtn, &m_shortcutResetNextBtn);

    m_shortcutResetAllBtn = new QPushButton(I18n::instance().tr("shortcutResetAll"), shortcutsBody);
    m_shortcutResetAllBtn->setObjectName("settingsLinkBtn");
    m_shortcutResetAllBtn->setCursor(Qt::PointingHandCursor);
    m_shortcutResetAllBtn->setFlat(true);
    connect(m_shortcutResetAllBtn, &QPushButton::clicked, this, [this]() {
        AppShortcuts::instance().resetAllAndSave();
        refreshShortcutEditors();
    });
    shortcutsLay->addWidget(m_shortcutResetAllBtn, 0, Qt::AlignLeft);

    m_shortcutStatusLabel = new QLabel(GlobalShortcutController::instance().statusText(), shortcutsBody);
    m_shortcutStatusLabel->setObjectName("settingsInfo");
    m_shortcutStatusLabel->setWordWrap(true);
    shortcutsLay->addWidget(m_shortcutStatusLabel);

    m_shortcutConfigureBtn = new QPushButton(I18n::instance().tr("shortcutOpenSystemSettings"), shortcutsBody);
    m_shortcutConfigureBtn->setObjectName("settingsLinkBtn");
    m_shortcutConfigureBtn->setCursor(Qt::PointingHandCursor);
    m_shortcutConfigureBtn->setFlat(true);
    connect(m_shortcutConfigureBtn, &QPushButton::clicked, this, []() {
        GlobalShortcutController::instance().openSystemConfigureUi();
    });
    shortcutsLay->addWidget(m_shortcutConfigureBtn, 0, Qt::AlignLeft);

    connect(&GlobalShortcutController::instance(), &GlobalShortcutController::bindingStateChanged, this,
            [this](bool, GlobalShortcutController::Backend) {
                if (m_shortcutStatusLabel)
                    m_shortcutStatusLabel->setText(GlobalShortcutController::instance().statusText());
            });

    m_shortcutHintLabel = new QLabel(I18n::instance().tr("shortcutWaylandHint"), shortcutsBody);
    m_shortcutHintLabel->setObjectName("settingsInfo");
    m_shortcutHintLabel->setWordWrap(true);
    shortcutsLay->addWidget(m_shortcutHintLabel);
    shortcutsLay->addStretch();

    QVBoxLayout *aboutLay = nullptr;
    QWidget *aboutCard = createSettingsCard(container, &aboutLay);
    QWidget *aboutBody = static_cast<GlassWidget *>(aboutCard)->contentWidget();

    // 关于
    m_aboutLabel = new QLabel(I18n::instance().tr("about"), aboutBody);
    m_aboutLabel->setObjectName("settingsLabel");
    aboutLay->addWidget(m_aboutLabel);

    // 版本 & 系统
    m_versionLabel = new QLabel(QString("%1: %2").arg(I18n::instance().version(), QString::fromUtf8(APP_VERSION)), aboutBody);
    m_versionLabel->setObjectName("settingsInfo");
    aboutLay->addWidget(m_versionLabel);

    m_systemLabel = new QLabel(QString("%1: %2").arg(I18n::instance().system()).arg(QSysInfo::prettyProductName()), aboutBody);
    m_systemLabel->setObjectName("settingsInfo");
    aboutLay->addWidget(m_systemLabel);

    auto *linksRow = new QHBoxLayout();
    linksRow->setSpacing(20);

    m_githubBtn = new QPushButton(I18n::instance().tr("githubRepo"), aboutBody);
    m_githubBtn->setObjectName("settingsLinkBtn");
    m_githubBtn->setCursor(Qt::PointingHandCursor);
    m_githubBtn->setFlat(true);
    connect(m_githubBtn, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl(QString::fromUtf8(kGithubUrl)));
    });
    linksRow->addWidget(m_githubBtn);

    m_apiDocsBtn = new QPushButton(I18n::instance().tr("apiDocs"), aboutBody);
    m_apiDocsBtn->setObjectName("settingsLinkBtn");
    m_apiDocsBtn->setCursor(Qt::PointingHandCursor);
    m_apiDocsBtn->setFlat(true);
    connect(m_apiDocsBtn, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl(QString::fromUtf8(kApiDocsUrl)));
    });
    linksRow->addWidget(m_apiDocsBtn);
    linksRow->addStretch();
    aboutLay->addLayout(linksRow);

    // 检查更新按钮
    m_checkUpdateBtn = new QPushButton(I18n::instance().tr("checkForUpdates"), aboutBody);
    m_checkUpdateBtn->setObjectName("checkUpdateBtn");
    m_checkUpdateBtn->setFixedHeight(40);
    m_checkUpdateBtn->setCursor(Qt::PointingHandCursor);
    connect(m_checkUpdateBtn, &QPushButton::clicked, this, &SettingsPage::checkForUpdatesRequested);
    aboutLay->addWidget(m_checkUpdateBtn);

    aboutLay->addStretch();

    m_settingsStack->addWidget(generalCard);
    m_settingsStack->addWidget(appearanceCard);
    m_settingsStack->addWidget(shortcutsCard);
    m_settingsStack->addWidget(aboutCard);
    setActiveSettingsTab(0);
    lay->addStretch();

    scroll->setWidget(container);
    nekoPolishScrollAreaViewport(scroll);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(scroll);
}

void SettingsPage::setupPersonalizationSection(QVBoxLayout *cardLay, QWidget *cardBody)
{
    m_personalizeSectionLabel = new QLabel(I18n::instance().tr("personalization"), cardBody);
    m_personalizeSectionLabel->setObjectName("settingsLabel");
    cardLay->addWidget(m_personalizeSectionLabel);

    auto *kindRow = new QHBoxLayout();
    m_backdropKindLabel = new QLabel(I18n::instance().tr("shellBackdrop"), cardBody);
    m_backdropKindLabel->setObjectName("settingsLabel");
    kindRow->addWidget(m_backdropKindLabel);
    kindRow->addStretch();

    m_backdropKindCombo = new QComboBox(cardBody);
    m_backdropKindCombo->setObjectName("settingsCombo");
    m_backdropKindCombo->addItem(I18n::instance().tr("shellBackdropDefaultImage"),
        static_cast<int>(ShellBackdropSettings::Kind::DefaultImage));
    m_backdropKindCombo->addItem(I18n::instance().tr("shellBackdropCustomImage"),
        static_cast<int>(ShellBackdropSettings::Kind::CustomImage));
    m_backdropKindCombo->addItem(I18n::instance().tr("shellBackdropSolidColor"),
        static_cast<int>(ShellBackdropSettings::Kind::SolidColor));

    auto &backdrop = ShellBackdropSettings::instance();
    for (int i = 0; i < m_backdropKindCombo->count(); ++i) {
        if (m_backdropKindCombo->itemData(i).toInt() == static_cast<int>(backdrop.kind())) {
            m_backdropKindCombo->setCurrentIndex(i);
            break;
        }
    }

    connect(m_backdropKindCombo, QOverload<int>::of(&QComboBox::activated), this, [this](int index) {
        const auto kind = static_cast<ShellBackdropSettings::Kind>(
            m_backdropKindCombo->itemData(index).toInt());
        ShellBackdropSettings::instance().setKind(kind);
        updateBackdropOptionRows();
    });
    kindRow->addWidget(m_backdropKindCombo);
    cardLay->addLayout(kindRow);

    m_backdropImageRow = new QWidget(cardBody);
    auto *imageLay = new QHBoxLayout(m_backdropImageRow);
    imageLay->setContentsMargins(0, 0, 0, 0);
    imageLay->setSpacing(12);

    m_backdropPickImageBtn = new QPushButton(I18n::instance().tr("shellBackdropPickImage"), m_backdropImageRow);
    m_backdropPickImageBtn->setObjectName("settingsLinkBtn");
    m_backdropPickImageBtn->setCursor(Qt::PointingHandCursor);
    m_backdropPickImageBtn->setFlat(true);
    connect(m_backdropPickImageBtn, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(
            window(), I18n::instance().tr("shellBackdropPickImage"), QString(),
            tr("Images (*.png *.jpg *.jpeg *.bmp *.webp)"));
        if (path.isEmpty())
            return;
        QPixmap probe;
        if (!probe.load(path)) {
            Toast::show(window(), I18n::instance().tr("shellBackdropImageInvalid"), Toast::Error);
            return;
        }
        auto &bd = ShellBackdropSettings::instance();
        bd.setCustomImagePath(path);
        bd.setKind(ShellBackdropSettings::Kind::CustomImage);
        for (int i = 0; i < m_backdropKindCombo->count(); ++i) {
            if (m_backdropKindCombo->itemData(i).toInt()
                == static_cast<int>(ShellBackdropSettings::Kind::CustomImage)) {
                m_backdropKindCombo->setCurrentIndex(i);
                break;
            }
        }
        refreshBackdropPathLabel();
        updateBackdropOptionRows();
    });
    imageLay->addWidget(m_backdropPickImageBtn);

    m_backdropResetImageBtn = new QPushButton(I18n::instance().tr("shellBackdropResetImage"), m_backdropImageRow);
    m_backdropResetImageBtn->setObjectName("settingsLinkBtn");
    m_backdropResetImageBtn->setCursor(Qt::PointingHandCursor);
    m_backdropResetImageBtn->setFlat(true);
    connect(m_backdropResetImageBtn, &QPushButton::clicked, this, [this]() {
        ShellBackdropSettings::instance().resetToDefaultImage();
        for (int i = 0; i < m_backdropKindCombo->count(); ++i) {
            if (m_backdropKindCombo->itemData(i).toInt()
                == static_cast<int>(ShellBackdropSettings::Kind::DefaultImage)) {
                m_backdropKindCombo->setCurrentIndex(i);
                break;
            }
        }
        refreshBackdropPathLabel();
        updateBackdropOptionRows();
    });
    imageLay->addWidget(m_backdropResetImageBtn);
    imageLay->addStretch();
    cardLay->addWidget(m_backdropImageRow);

    m_backdropPathLabel = new QLabel(cardBody);
    m_backdropPathLabel->setObjectName("settingsInfo");
    m_backdropPathLabel->setWordWrap(true);
    cardLay->addWidget(m_backdropPathLabel);

    m_backdropSolidRow = new QWidget(cardBody);
    auto *solidLay = new QHBoxLayout(m_backdropSolidRow);
    solidLay->setContentsMargins(0, 0, 0, 0);
    solidLay->setSpacing(12);

    m_backdropPickColorBtn = new QPushButton(I18n::instance().tr("shellBackdropPickColor"), m_backdropSolidRow);
    m_backdropPickColorBtn->setObjectName("settingsLinkBtn");
    m_backdropPickColorBtn->setCursor(Qt::PointingHandCursor);
    m_backdropPickColorBtn->setFlat(true);
    connect(m_backdropPickColorBtn, &QPushButton::clicked, this, [this]() {
        const QColor picked = QColorDialog::getColor(
            ShellBackdropSettings::instance().solidColor(), window(),
            I18n::instance().tr("shellBackdropPickColor"));
        if (!picked.isValid())
            return;
        ShellBackdropSettings::instance().setSolidColor(picked);
        refreshBackdropColorSwatch();
    });
    solidLay->addWidget(m_backdropPickColorBtn);

    m_backdropColorSwatch = new QLabel(m_backdropSolidRow);
    m_backdropColorSwatch->setFixedSize(28, 28);
    m_backdropColorSwatch->setObjectName("settingsColorSwatch");
    solidLay->addWidget(m_backdropColorSwatch);
    solidLay->addStretch();
    cardLay->addWidget(m_backdropSolidRow);

    refreshBackdropPathLabel();
    refreshBackdropColorSwatch();
    updateBackdropOptionRows();
}

void SettingsPage::updateBackdropOptionRows()
{
    const auto kind = ShellBackdropSettings::instance().kind();
    const bool imageMode = kind != ShellBackdropSettings::Kind::SolidColor;
    if (m_backdropImageRow)
        m_backdropImageRow->setVisible(imageMode);
    if (m_backdropPathLabel)
        m_backdropPathLabel->setVisible(imageMode);
    if (m_backdropSolidRow)
        m_backdropSolidRow->setVisible(!imageMode);
    if (m_backdropResetImageBtn)
        m_backdropResetImageBtn->setEnabled(kind == ShellBackdropSettings::Kind::CustomImage);
}

void SettingsPage::refreshBackdropPathLabel()
{
    if (!m_backdropPathLabel)
        return;
    auto &backdrop = ShellBackdropSettings::instance();
    if (backdrop.kind() == ShellBackdropSettings::Kind::DefaultImage) {
        m_backdropPathLabel->setText(I18n::instance().tr("shellBackdropUsingDefault"));
        return;
    }
    if (backdrop.customImagePath().isEmpty()) {
        m_backdropPathLabel->setText(I18n::instance().tr("shellBackdropNoCustomImage"));
        return;
    }
    m_backdropPathLabel->setText(
        QStringLiteral("%1: %2").arg(I18n::instance().tr("shellBackdropCurrentFile"),
                                       backdrop.customImagePath()));
}

void SettingsPage::refreshBackdropColorSwatch()
{
    if (!m_backdropColorSwatch)
        return;
    const QColor c = ShellBackdropSettings::instance().solidColor();
    m_backdropColorSwatch->setStyleSheet(
        QStringLiteral("#settingsColorSwatch { background: %1; border: 1px solid rgba(255,255,255,0.2);"
                       " border-radius: 6px; }")
            .arg(c.name(QColor::HexRgb)));
}

void SettingsPage::setupShortcutRow(QVBoxLayout *parentLayout, QWidget *cardBody, AppShortcuts::Action action,
                                    QLabel **labelOut, ShortcutCaptureButton **captureOut,
                                    QPushButton **resetOut)
{
    QString labelKey;
    switch (action) {
    case AppShortcuts::PlayPause:
        labelKey = QStringLiteral("shortcutPlayPause");
        break;
    case AppShortcuts::NextTrack:
        labelKey = QStringLiteral("shortcutNextTrack");
        break;
    case AppShortcuts::PreviousTrack:
        labelKey = QStringLiteral("shortcutPreviousTrack");
        break;
    default:
        break;
    }

    auto *row = new QHBoxLayout();
    *labelOut = new QLabel(I18n::instance().tr(labelKey), cardBody);
    (*labelOut)->setObjectName("settingsLabel");
    row->addWidget(*labelOut);
    row->addStretch();

    *captureOut = new ShortcutCaptureButton(cardBody);
    (*captureOut)->setKeySequence(AppShortcuts::instance().sequence(action));
    connect(*captureOut, &ShortcutCaptureButton::keySequenceChanged, this,
            [this, action, captureOut](const QKeySequence &seq) {
                applyShortcutChange(action, seq, *captureOut);
            });
    row->addWidget(*captureOut);

    *resetOut = new QPushButton(I18n::instance().tr("shortcutResetDefault"), cardBody);
    (*resetOut)->setObjectName("settingsLinkBtn");
    (*resetOut)->setCursor(Qt::PointingHandCursor);
    (*resetOut)->setFlat(true);
    connect(*resetOut, &QPushButton::clicked, this, [this, action, captureOut]() {
        const QKeySequence seq = AppShortcuts::defaultSequence(action);
        (*captureOut)->setKeySequence(seq);
        applyShortcutChange(action, seq, *captureOut);
    });
    row->addWidget(*resetOut);
    parentLayout->addLayout(row);
}

void SettingsPage::applyShortcutChange(AppShortcuts::Action action, const QKeySequence &seq,
                                       ShortcutCaptureButton *editor)
{
    if (!seq.isEmpty()) {
        for (int i = 0; i < AppShortcuts::ActionCount; ++i) {
            const auto other = static_cast<AppShortcuts::Action>(i);
            if (other == action)
                continue;
            if (AppShortcuts::instance().sequence(other) == seq) {
                editor->setKeySequence(AppShortcuts::instance().sequence(action));
                Toast::show(window(), I18n::instance().tr("shortcutConflict").arg(seq.toString(QKeySequence::NativeText)),
                            Toast::Error);
                return;
            }
        }
    }
    AppShortcuts::instance().setSequence(action, seq);
}

void SettingsPage::refreshShortcutEditors()
{
    if (m_shortcutPlayPauseBtn)
        m_shortcutPlayPauseBtn->setKeySequence(AppShortcuts::instance().sequence(AppShortcuts::PlayPause));
    if (m_shortcutPrevBtn)
        m_shortcutPrevBtn->setKeySequence(AppShortcuts::instance().sequence(AppShortcuts::PreviousTrack));
    if (m_shortcutNextBtn)
        m_shortcutNextBtn->setKeySequence(AppShortcuts::instance().sequence(AppShortcuts::NextTrack));
}

void SettingsPage::retranslate()
{
    if (m_titleLabel)
        m_titleLabel->setText(I18n::instance().settingsTitle());
    if (m_descLabel)
        m_descLabel->setText(I18n::instance().tr(QStringLiteral("settingsDesc")));
    if (m_generalTabBtn)
        m_generalTabBtn->setText(I18n::instance().tr(QStringLiteral("settingsTabGeneral")));
    if (m_appearanceTabBtn)
        m_appearanceTabBtn->setText(I18n::instance().tr(QStringLiteral("settingsTabAppearance")));
    if (m_shortcutsTabBtn)
        m_shortcutsTabBtn->setText(I18n::instance().tr(QStringLiteral("settingsTabShortcuts")));
    if (m_aboutTabBtn)
        m_aboutTabBtn->setText(I18n::instance().tr(QStringLiteral("settingsTabAbout")));
    m_langLabel->setText(I18n::instance().languageLabel());
    m_langCombo->setItemText(0, I18n::instance().languageChinese());
    m_langCombo->setItemText(1, I18n::instance().languageNya());
    m_langCombo->setItemText(2, I18n::instance().languageEnglish());
    if (m_themeLabel)
        m_themeLabel->setText(I18n::instance().tr("theme"));
    if (m_themeCombo) {
        m_themeCombo->setItemText(0, I18n::instance().tr("themeSystem"));
        m_themeCombo->setItemText(1, I18n::instance().tr("themeDark"));
        m_themeCombo->setItemText(2, I18n::instance().tr("themeLight"));
    }
    if (m_personalizeSectionLabel)
        m_personalizeSectionLabel->setText(I18n::instance().tr("personalization"));
    if (m_backdropKindLabel)
        m_backdropKindLabel->setText(I18n::instance().tr("shellBackdrop"));
    if (m_backdropKindCombo) {
        m_backdropKindCombo->setItemText(0, I18n::instance().tr("shellBackdropDefaultImage"));
        m_backdropKindCombo->setItemText(1, I18n::instance().tr("shellBackdropCustomImage"));
        m_backdropKindCombo->setItemText(2, I18n::instance().tr("shellBackdropSolidColor"));
    }
    if (m_backdropPickImageBtn)
        m_backdropPickImageBtn->setText(I18n::instance().tr("shellBackdropPickImage"));
    if (m_backdropResetImageBtn)
        m_backdropResetImageBtn->setText(I18n::instance().tr("shellBackdropResetImage"));
    if (m_backdropPickColorBtn)
        m_backdropPickColorBtn->setText(I18n::instance().tr("shellBackdropPickColor"));
    refreshBackdropPathLabel();
    refreshBackdropColorSwatch();
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
    if (m_shortcutsSectionLabel)
        m_shortcutsSectionLabel->setText(I18n::instance().tr("shortcuts"));
    if (m_shortcutPlayPauseLabel)
        m_shortcutPlayPauseLabel->setText(I18n::instance().tr("shortcutPlayPause"));
    if (m_shortcutPrevLabel)
        m_shortcutPrevLabel->setText(I18n::instance().tr("shortcutPreviousTrack"));
    if (m_shortcutNextLabel)
        m_shortcutNextLabel->setText(I18n::instance().tr("shortcutNextTrack"));
    if (m_shortcutHintLabel)
        m_shortcutHintLabel->setText(I18n::instance().tr("shortcutWaylandHint"));
    if (m_shortcutStatusLabel)
        m_shortcutStatusLabel->setText(GlobalShortcutController::instance().statusText());
    if (m_shortcutConfigureBtn)
        m_shortcutConfigureBtn->setText(I18n::instance().tr("shortcutOpenSystemSettings"));
    if (m_shortcutResetAllBtn)
        m_shortcutResetAllBtn->setText(I18n::instance().tr("shortcutResetAll"));
    if (m_shortcutResetPlayPauseBtn)
        m_shortcutResetPlayPauseBtn->setText(I18n::instance().tr("shortcutResetDefault"));
    if (m_shortcutResetPrevBtn)
        m_shortcutResetPrevBtn->setText(I18n::instance().tr("shortcutResetDefault"));
    if (m_shortcutResetNextBtn)
        m_shortcutResetNextBtn->setText(I18n::instance().tr("shortcutResetDefault"));
    refreshShortcutEditors();
}

void SettingsPage::paintEvent(QPaintEvent *) {}
