#pragma once

#include <QString>

class FileDetector
{
public:
    enum class ImageFormat {
        PNG,
        AVIF,
        HEIC,
        JPEG_XL,
        JPEG,
        TIFF,
        Unknown
    };

    static bool isImageHDR(const QString &imagePath);
    static bool isSupportedImageFormat(const QString &filePath);
    static bool hasImageExtension(const QString &filePath);
    static ImageFormat detectImageFormat(const QString &filePath);

private:
    static bool isPngHDR(const QString &filePath);
    static bool isAvifHDR(const QString &filePath);
    static bool isHeicHDR(const QString &filePath);
    static bool isJpegXlHDR(const QString &filePath);
    static bool isJpegHDR(const QString &filePath);
    static bool isTiffHDR(const QString &filePath);
};
