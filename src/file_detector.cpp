#include "file_detector.h"

#include <QFile>
#include <QDebug>
#include <QUrl>
#include <QFileInfo>

bool FileDetector::hasImageExtension(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    QString ext = fileInfo.suffix().toLower();
    
    // List of supported image extensions (for fast directory filtering)
    static const QSet<QString> imageExtensions = {
        QStringLiteral("png"),
        QStringLiteral("jpg"), QStringLiteral("jpeg"),
        QStringLiteral("avif"),
        QStringLiteral("heic"), QStringLiteral("heif"), QStringLiteral("hif"),
        QStringLiteral("jxl"),
        QStringLiteral("tiff"), QStringLiteral("tif")
    };
    
    return imageExtensions.contains(ext);
}

bool FileDetector::isSupportedImageFormat(const QString &filePath)
{
    // For navigation: just check file extension (fast)
    return hasImageExtension(filePath);
}

bool FileDetector::isImageHDR(const QString &imagePath)
{
    // Convert URL to local file path if needed
    QString localPath = imagePath;
    if (localPath.startsWith(QStringLiteral("file://"))) {
        localPath = QUrl(imagePath).toLocalFile();
    }

    // ALWAYS use magic bytes for HDR detection (no extension checking)
    ImageFormat format = detectImageFormat(localPath);
    
    QString formatName;
    bool isHDR = false;
    
    switch (format) {
        case ImageFormat::PNG:
            formatName = QStringLiteral("PNG");
            isHDR = isPngHDR(localPath);
            break;
        case ImageFormat::AVIF:
            formatName = QStringLiteral("AVIF");
            isHDR = isAvifHDR(localPath);
            break;
        case ImageFormat::HEIC:
            formatName = QStringLiteral("HEIC");
            isHDR = isHeicHDR(localPath);
            break;
        case ImageFormat::JPEG_XL:
            formatName = QStringLiteral("JPEG-XL");
            isHDR = isJpegXlHDR(localPath);
            break;
        case ImageFormat::JPEG:
            formatName = QStringLiteral("JPEG");
            isHDR = isJpegHDR(localPath);
            break;
        case ImageFormat::TIFF:
            formatName = QStringLiteral("TIFF");
            isHDR = isTiffHDR(localPath);
            break;
        case ImageFormat::Unknown:
            qWarning() << "Unknown image format (magic bytes not recognized):" << localPath;
            return false;
    }
    
    qDebug() << "Detected format (via magic bytes):" << formatName << "| HDR:" << (isHDR ? "Yes" : "No") << "| File:" << localPath;
    
    return isHDR;
}

FileDetector::ImageFormat FileDetector::detectImageFormat(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open file for format detection:" << filePath;
        return ImageFormat::Unknown;
    }

    // Read first 12 bytes for magic number detection
    QByteArray header = file.read(12);
    file.close();

    if (header.size() < 4) {
        return ImageFormat::Unknown;
    }

    // PNG: 89 50 4E 47 0D 0A 1A 0A
    if (header.size() >= 8 &&
        static_cast<unsigned char>(header[0]) == 0x89 &&
        header[1] == 0x50 &&
        header[2] == 0x4E &&
        header[3] == 0x47 &&
        static_cast<unsigned char>(header[4]) == 0x0D &&
        static_cast<unsigned char>(header[5]) == 0x0A &&
        static_cast<unsigned char>(header[6]) == 0x1A &&
        static_cast<unsigned char>(header[7]) == 0x0A) {
        return ImageFormat::PNG;
    }

    // JPEG: FF D8 FF
    if (header.size() >= 3 &&
        static_cast<unsigned char>(header[0]) == 0xFF &&
        static_cast<unsigned char>(header[1]) == 0xD8 &&
        static_cast<unsigned char>(header[2]) == 0xFF) {
        return ImageFormat::JPEG;
    }

    // TIFF: 49 49 2A 00 (little-endian) or 4D 4D 00 2A (big-endian)
    if (header.size() >= 4 &&
        ((header[0] == 0x49 && header[1] == 0x49 && header[2] == 0x2A && header[3] == 0x00) ||
         (header[0] == 0x4D && header[1] == 0x4D && header[2] == 0x00 && header[3] == 0x2A))) {
        return ImageFormat::TIFF;
    }

    // AVIF/HEIC: Both use ISO Base Media File Format (ftyp box)
    // Check for "ftyp" at bytes 4-7
    if (header.size() >= 12 &&
        header[4] == 0x66 &&  // 'f'
        header[5] == 0x74 &&  // 't'
        header[6] == 0x79 &&  // 'y'
        header[7] == 0x70) {  // 'p'
        
        // Check brand at bytes 8-11
        // AVIF: "avif", "avis"
        if ((header[8] == 0x61 && header[9] == 0x76 && header[10] == 0x69 && header[11] == 0x66) ||
            (header[8] == 0x61 && header[9] == 0x76 && header[10] == 0x69 && header[11] == 0x73)) {
            return ImageFormat::AVIF;
        }
        
        // HEIC: "heic", "heix", "hevc", "hevx", "mif1"
        if ((header[8] == 0x68 && header[9] == 0x65 && header[10] == 0x69 && header[11] == 0x63) ||
            (header[8] == 0x68 && header[9] == 0x65 && header[10] == 0x69 && header[11] == 0x78) ||
            (header[8] == 0x68 && header[9] == 0x65 && header[10] == 0x76 && header[11] == 0x63) ||
            (header[8] == 0x68 && header[9] == 0x65 && header[10] == 0x76 && header[11] == 0x78) ||
            (header[8] == 0x6D && header[9] == 0x69 && header[10] == 0x66 && header[11] == 0x31)) {
            return ImageFormat::HEIC;
        }
    }

    // JPEG-XL: FF 0A or 00 00 00 0C 4A 58 4C 20 0D 0A 87 0A (container format)
    if (header.size() >= 2 &&
        static_cast<unsigned char>(header[0]) == 0xFF &&
        static_cast<unsigned char>(header[1]) == 0x0A) {
        return ImageFormat::JPEG_XL;
    }
    
    if (header.size() >= 12 &&
        header[0] == 0x00 && header[1] == 0x00 && header[2] == 0x00 && header[3] == 0x0C &&
        header[4] == 0x4A && header[5] == 0x58 && header[6] == 0x4C && header[7] == 0x20 &&
        static_cast<unsigned char>(header[8]) == 0x0D &&
        static_cast<unsigned char>(header[9]) == 0x0A &&
        static_cast<unsigned char>(header[10]) == 0x87 &&
        static_cast<unsigned char>(header[11]) == 0x0A) {
        return ImageFormat::JPEG_XL;
    }

    return ImageFormat::Unknown;
}

bool FileDetector::isPngHDR(const QString &filePath)
{
    // TODO: Implement PNG HDR detection (check bit depth, color space metadata)
    return false;
}

bool FileDetector::isAvifHDR(const QString &filePath)
{
    // TODO: Implement AVIF HDR detection (check color space, transfer characteristics)
    return false;
}

bool FileDetector::isHeicHDR(const QString &filePath)
{
    // TODO: Implement HEIC HDR detection (check color space, transfer characteristics)
    return false;
}

bool FileDetector::isJpegXlHDR(const QString &filePath)
{
    // TODO: Implement JPEG-XL HDR detection (check color encoding, bit depth)
    return false;
}

bool FileDetector::isJpegHDR(const QString &filePath)
{
    // JPEG is always SDR
    return false;
}

bool FileDetector::isTiffHDR(const QString &filePath)
{
    // TODO: Implement TIFF HDR detection (check bit depth, PhotometricInterpretation)
    return false;
}
