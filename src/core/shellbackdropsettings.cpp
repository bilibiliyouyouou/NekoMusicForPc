#include "shellbackdropsettings.h"

#include <QSettings>

namespace {

constexpr auto kKeyKind = "shellBackdrop/kind";
constexpr auto kKeyCustomPath = "shellBackdrop/customPath";
constexpr auto kKeySolidColor = "shellBackdrop/solidColor";
constexpr const char *kDefaultResourcePath = ":/background-pages.jpg";

} // namespace

ShellBackdropSettings &ShellBackdropSettings::instance()
{
    static ShellBackdropSettings inst;
    return inst;
}

ShellBackdropSettings::ShellBackdropSettings(QObject *parent)
    : QObject(parent)
{
    load();
}

void ShellBackdropSettings::load()
{
    QSettings s;
    m_kind = static_cast<Kind>(s.value(kKeyKind, static_cast<int>(Kind::DefaultImage)).toInt());
    m_customPath = s.value(kKeyCustomPath).toString();
    const QColor c(s.value(kKeySolidColor, QStringLiteral("#1a1a22")).toString());
    m_solidColor = c.isValid() ? c : QColor(26, 26, 34);
}

void ShellBackdropSettings::save() const
{
    QSettings s;
    s.setValue(kKeyKind, static_cast<int>(m_kind));
    s.setValue(kKeyCustomPath, m_customPath);
    s.setValue(kKeySolidColor, m_solidColor.name(QColor::HexRgb));
}

bool ShellBackdropSettings::usesImageBackdrop() const
{
    return m_kind == Kind::DefaultImage || m_kind == Kind::CustomImage;
}

QPixmap ShellBackdropSettings::sourcePixmap() const
{
    if (!usesImageBackdrop())
        return {};

    if (m_kind == Kind::CustomImage && !m_customPath.isEmpty()) {
        QPixmap pm;
        if (pm.load(m_customPath))
            return pm;
    }

    QPixmap pm;
    if (pm.load(QLatin1String(kDefaultResourcePath)))
        return pm;
    return {};
}

void ShellBackdropSettings::setKind(Kind kind)
{
    if (m_kind == kind)
        return;
    m_kind = kind;
    save();
    emit changed();
}

void ShellBackdropSettings::setCustomImagePath(const QString &path)
{
    if (m_customPath == path)
        return;
    m_customPath = path;
    save();
    emit changed();
}

void ShellBackdropSettings::setSolidColor(const QColor &color)
{
    if (!color.isValid() || m_solidColor == color)
        return;
    m_solidColor = color;
    save();
    emit changed();
}

void ShellBackdropSettings::resetToDefaultImage()
{
    m_customPath.clear();
    m_kind = Kind::DefaultImage;
    save();
    emit changed();
}
