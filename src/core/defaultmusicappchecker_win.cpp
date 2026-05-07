#include "defaultmusicappchecker.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QDesktopServices>

#include <windows.h>
#include <shlwapi.h>

namespace {

QString assocExecutableForExtension(const wchar_t *extWithDot)
{
    wchar_t buf[MAX_PATH * 4] = {};
    DWORD cch = DWORD(sizeof(buf) / sizeof(buf[0]));
    HRESULT hr = AssocQueryStringW(ASSOCF_INIT_IGNOREUNKNOWN, ASSOCSTR_EXECUTABLE, extWithDot, nullptr, buf, &cch);
    if (FAILED(hr) || !buf[0])
        return {};
    return QString::fromWCharArray(buf);
}

QString normalizeExe(const QString &path)
{
    if (path.isEmpty())
        return {};
    QFileInfo fi(path);
    QString c = fi.canonicalFilePath();
    if (c.isEmpty())
        c = fi.absoluteFilePath();
    return QDir::toNativeSeparators(c).toLower();
}

bool extensionDefaultsToOurExe(const wchar_t *ext)
{
    const QString handler = normalizeExe(assocExecutableForExtension(ext));
    if (handler.isEmpty())
        return false;
    const QString self = normalizeExe(QCoreApplication::applicationFilePath());
    if (self.isEmpty())
        return false;
    return handler == self;
}

} // namespace

namespace DefaultMusicAppChecker {

bool isDefaultMusicPlayer()
{
    // 常见后缀与 nekomusic.desktop 中类型对应
    const wchar_t *exts[] = {L".mp3", L".flac", L".wav", L".ogg"};
    int ok = 0;
    int total = 0;
    for (const wchar_t *e : exts) {
        QString h = normalizeExe(assocExecutableForExtension(e));
        if (h.isEmpty())
            continue;
        total++;
        if (extensionDefaultsToOurExe(e))
            ok++;
    }
    if (total == 0)
        return true;
    return ok == total;
}

void trySetAsDefaultMusicPlayer()
{
    // 无稳定公开 API 一键写入「音乐」总类：打开系统「默认应用」页，由用户选择
    QDesktopServices::openUrl(QUrl(QStringLiteral("ms-settings:defaultapps")));
}

} // namespace DefaultMusicAppChecker
