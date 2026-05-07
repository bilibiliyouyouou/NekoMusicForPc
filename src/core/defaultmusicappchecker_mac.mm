#include "defaultmusicappchecker.h"

#import <Foundation/Foundation.h>
#import <CoreServices/CoreServices.h>

namespace {

NSString *defaultBundleIdForUti(NSString *uti)
{
    if (!uti.length)
        return nil;
    CFStringRef bid = LSCopyDefaultRoleHandlerForContentType((__bridge CFStringRef)uti, kLSRolesViewer);
    if (!bid)
        bid = LSCopyDefaultRoleHandlerForContentType((__bridge CFStringRef)uti, kLSRolesAll);
    if (!bid)
        return nil;
    return (__bridge_transfer NSString *)bid;
}

NSString *ourBundleId()
{
    return [[NSBundle mainBundle] bundleIdentifier];
}

bool utiDefaultsToUs(NSString *uti)
{
    NSString *def = defaultBundleIdForUti(uti);
    if (!def.length)
        return false;
    NSString *mine = ourBundleId();
    if (!mine.length)
        return false;
    return [def isEqualToString:mine];
}

} // namespace

namespace DefaultMusicAppChecker {

bool isDefaultMusicPlayer()
{
    // 与桌面 MimeType / 常见后缀对应的 UTI（部分冷门格式系统可能无登记，query 为空则跳过）
    NSArray *utis = @[
        @"public.mp3",
        @"org.xiph.flac",
        @"com.microsoft.waveform-audio",
        @"public.ogg-audio",
        @"public.aac-audio",
        @"public.mpeg-4-audio",
        @"com.microsoft.windows-media-wma",
        @"org.xiph.speex",
        @"com.real.realmedia",
        @"public.playlist",
    ];
    NSInteger total = 0;
    NSInteger ok = 0;
    for (NSString *uti in utis) {
        NSString *def = defaultBundleIdForUti(uti);
        if (!def.length)
            continue;
        total++;
        if (utiDefaultsToUs(uti))
            ok++;
    }
    if (total == 0)
        return true;
    return ok == total;
}

void trySetAsDefaultMusicPlayer()
{
    NSString *bid = ourBundleId();
    if (!bid.length)
        return;
    NSArray *utis = @[
        @"public.mp3",
        @"org.xiph.flac",
        @"com.microsoft.waveform-audio",
        @"public.ogg-audio",
        @"public.aac-audio",
        @"public.mpeg-4-audio",
        @"com.microsoft.windows-media-wma",
        @"org.xiph.speex",
        @"com.real.realmedia",
        @"public.playlist",
    ];
    for (NSString *uti in utis) {
        LSSetDefaultRoleHandlerForContentType((__bridge CFStringRef)uti, kLSRolesViewer, (__bridge CFStringRef)bid);
        LSSetDefaultRoleHandlerForContentType((__bridge CFStringRef)uti, kLSRolesEditor, (__bridge CFStringRef)bid);
    }
}

} // namespace DefaultMusicAppChecker
