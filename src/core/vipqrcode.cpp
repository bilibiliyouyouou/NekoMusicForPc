/**
 * @file vipqrcode.cpp
 */

#include "vipqrcode.h"

#include "qrcodegen.hpp"

#include <QImage>
#include <QPainter>
#include <QRect>

namespace VipQrCode {

QPixmap pixmapFromText(const QString &text, int pixelSize)
{
    const QByteArray utf8 = text.trimmed().toUtf8();
    if (utf8.isEmpty() || pixelSize < 32)
        return {};

    try {
        const qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(
            utf8.constData(), qrcodegen::QrCode::Ecc::MEDIUM);
        const int modules = qr.getSize();
        if (modules <= 0)
            return {};

        const int border = 2;
        const int totalModules = modules + border * 2;
        const int scale = qMax(1, pixelSize / totalModules);
        const int imgSize = totalModules * scale;

        QImage image(imgSize, imgSize, QImage::Format_RGB32);
        image.fill(Qt::white);

        const QColor dark(0x1a, 0x1a, 0x1a);
        QPainter p(&image);
        for (int y = 0; y < modules; ++y) {
            for (int x = 0; x < modules; ++x) {
                if (!qr.getModule(x, y))
                    continue;
                p.fillRect(QRect((x + border) * scale, (y + border) * scale, scale, scale), dark);
            }
        }
        return QPixmap::fromImage(image);
    } catch (...) {
        return {};
    }
}

} // namespace VipQrCode
