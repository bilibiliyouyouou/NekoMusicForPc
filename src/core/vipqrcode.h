#pragma once

/**
 * @file vipqrcode.h
 * @brief 将支付链接编码为二维码 QPixmap（基于 qrcodegen）
 */

#include <QPixmap>
#include <QString>

namespace VipQrCode {

/** 将文本编码为二维码图像；失败返回空 QPixmap。 */
QPixmap pixmapFromText(const QString &text, int pixelSize = 220);

} // namespace VipQrCode
