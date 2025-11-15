#include "file_detector.h"

#include <QFile>
#include <QDebug>
#include <QUrl>
#include <QFileInfo>
#include <jxl/decode.h>
#include <jxl/decode_cxx.h>
#include <jxl/types.h>

// Helper function to read big-endian 32-bit integer
static quint32 readBigEndian32(const QByteArray &data, int offset) {
    if (offset + 4 > data.size()) return 0;
    return (static_cast<unsigned char>(data[offset]) << 24) |
           (static_cast<unsigned char>(data[offset + 1]) << 16) |
           (static_cast<unsigned char>(data[offset + 2]) << 8) |
           static_cast<unsigned char>(data[offset + 3]);
}

// Helper function to parse ISO Base Media File Format boxes (used by AVIF and HEIC)
static bool parseIsoMediaBoxesForHDR(QFile &file, qint64 maxEnd = -1) {
    if (maxEnd == -1) {
        maxEnd = file.size();
    }
    
    while (file.pos() < maxEnd && !file.atEnd()) {
        qint64 boxStart = file.pos();
        
        // Read box size (4 bytes)
        QByteArray sizeBytes = file.read(4);
        if (sizeBytes.size() < 4) break;
        
        quint32 boxSize = readBigEndian32(sizeBytes, 0);
        
        // Read box type (4 bytes)
        QByteArray boxType = file.read(4);
        if (boxType.size() < 4) break;
        
        // Handle extended size
        qint64 actualSize = boxSize;
        if (boxSize == 1) {
            QByteArray extSizeBytes = file.read(8);
            if (extSizeBytes.size() < 8) break;
            actualSize = 0;
            for (int i = 0; i < 8; i++) {
                actualSize = (actualSize << 8) | static_cast<unsigned char>(extSizeBytes[i]);
            }
        } else if (boxSize == 0) {
            actualSize = maxEnd - boxStart;
        }
        
        qint64 dataStart = file.pos();
        qint64 dataEnd = boxStart + actualSize;
        
        // Check for 'colr' box (Color Information Box)
        if (boxType == "colr") {
            QByteArray colrData = file.read(qMin(actualSize - 8, (qint64)20));
            
            if (colrData.size() >= 11) {
                // colr box structure:
                // 4 bytes: color_type (e.g., "nclx" for MPEG color parameters, "prof" for ICC profile)
                QString colorType = QString::fromLatin1(colrData.left(4));
                
                if (colorType == "nclx" || colorType == "rncl") { // Some encoders write it reversed
                    // NCLX color parameters:
                    // 2 bytes: color_primaries
                    // 2 bytes: transfer_characteristics
                    // 2 bytes: matrix_coefficients
                    // 1 byte: full_range_flag
                    
                    if (colrData.size() >= 11) {
                        quint16 colorPrimaries = (static_cast<unsigned char>(colrData[4]) << 8) |
                                                 static_cast<unsigned char>(colrData[5]);
                        quint16 transferCharacteristics = (static_cast<unsigned char>(colrData[6]) << 8) |
                                                          static_cast<unsigned char>(colrData[7]);
                        
                        // Transfer Characteristics = 16 is PQ (SMPTE ST 2084)
                        // Color Primaries = 9 is BT.2020
                        if (transferCharacteristics == 16 || colorPrimaries == 9) {
                            return true;
                        }
                    }
                } else if (colorType == "prof") {
                    // ICC profile embedded - search for HDR indicators in the profile
                    qint64 savedPos = file.pos();
                    // Read more of the profile - skip the first 4 bytes (which is the colorType "prof")
                    // and read up to 8KB of the profile
                    QByteArray profileData = file.read(qMin(actualSize - 8, (qint64)8192));
                    QString profileStr = QString::fromLatin1(profileData);
                    
                    // Look for Rec. 2020 PQ profile name
                    if (profileStr.contains("Rec. 2020 PQ", Qt::CaseInsensitive) || 
                        profileStr.contains("Rec. 2020", Qt::CaseInsensitive) ||
                        profileStr.contains("BT.2020", Qt::CaseInsensitive)) {
                        return true;
                    }
                    
                    // Also check for "2020" and "PQ" separately  
                    if (profileStr.contains("2020") && profileStr.contains("PQ")) {
                        return true;
                    }
                    
                    file.seek(savedPos);
                }
            }
        }
        
        // Recursively parse container boxes
        if (boxType == "meta" || boxType == "iprp" || boxType == "ipco" || 
            boxType == "moov" || boxType == "trak" || boxType == "mdia") {
            qint64 savedPos = file.pos();
            
            // 'meta' box has 4 bytes version/flags, skip them
            if (boxType == "meta") {
                file.seek(savedPos + 4);
            }
            
            if (parseIsoMediaBoxesForHDR(file, dataEnd)) {
                return true;
            }
        }
        
        // Move to next box
        if (dataEnd > file.pos()) {
            file.seek(dataEnd);
        } else {
            break;
        }
    }
    
    return false;
}


bool FileDetector::isSupportedImageFormat(const QString &filePath)
{
    ImageFormat format = detectImageFormat(filePath);
    return format != ImageFormat::Unknown;
}

bool FileDetector::isImageHDR(const QString &imagePath)
{
    // Convert URL to local file path if needed
    QString localPath = imagePath;
    if (localPath.startsWith(QStringLiteral("file://"))) {
        localPath = QUrl(imagePath).toLocalFile();
    }

    // Use magic bytes for format detection
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
    // PNG HDR detection: Check for cICP chunk (color information) or iCCP profile name
    // cICP chunk format: Color Primaries (1 byte), Transfer Characteristics (1 byte), Matrix Coefficients (1 byte), Full Range Flag (1 byte)
    // Transfer Characteristics = 16 indicates PQ (HDR10/HDR)
    // Also check iCCP profile name for "PQ" or "Rec. 2020"
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    // Skip PNG signature (8 bytes)
    file.seek(8);
    
    while (!file.atEnd()) {
        // Read chunk length (4 bytes, big-endian)
        QByteArray lengthBytes = file.read(4);
        if (lengthBytes.size() < 4) break;
        
        quint32 chunkLength = (static_cast<unsigned char>(lengthBytes[0]) << 24) |
                              (static_cast<unsigned char>(lengthBytes[1]) << 16) |
                              (static_cast<unsigned char>(lengthBytes[2]) << 8) |
                              static_cast<unsigned char>(lengthBytes[3]);
        
        // Read chunk type (4 bytes)
        QByteArray chunkType = file.read(4);
        if (chunkType.size() < 4) break;
        
        // Check for cICP chunk (Coding-Independent Code Points)
        if (chunkType == "cICP") {
            QByteArray chunkData = file.read(qMin(chunkLength, 4u));
            if (chunkData.size() >= 2) {
                // Byte 1: Transfer Characteristics
                unsigned char transferCharacteristics = static_cast<unsigned char>(chunkData[1]);
                // Transfer Characteristics = 16 is PQ (SMPTE ST 2084)
                if (transferCharacteristics == 16) {
                    file.close();
                    return true;
                }
            }
        }
        
        // Check for iCCP chunk (embedded ICC profile)
        if (chunkType == "iCCP") {
            QByteArray chunkData = file.read(chunkLength);
            QString profileData = QString::fromLatin1(chunkData);
            
            // Check if profile name contains HDR indicators
            if (profileData.contains("PQ", Qt::CaseInsensitive) || 
                profileData.contains("Rec. 2020", Qt::CaseInsensitive) ||
                profileData.contains("BT.2020", Qt::CaseInsensitive)) {
                file.close();
                return true;
            }
            
            // Skip CRC (already read as part of chunk data if needed)
            file.seek(file.pos() + 4);
            continue;
        }
        
        // Check for IEND chunk (end of PNG)
        if (chunkType == "IEND") {
            break;
        }
        
        // Skip chunk data and CRC (4 bytes)
        file.seek(file.pos() + chunkLength + 4);
    }
    
    file.close();
    return false;
}

bool FileDetector::isAvifHDR(const QString &filePath)
{
    // AVIF HDR detection: Parse ISO Base Media File Format (MP4 container)
    // Look for 'colr' box (Color Information Box) inside 'ipco' (Item Property Container)
    // Transfer Characteristics = 16 (PQ) or Color Primaries = 9 (BT.2020) indicates HDR
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    // Parse boxes recursively
    bool isHDR = parseIsoMediaBoxesForHDR(file);
    
    file.close();
    return isHDR;
}

bool FileDetector::isHeicHDR(const QString &filePath)
{
    // HEIC uses the same format as AVIF (ISO Base Media File Format)
    // Both use the same HDR detection method
    return isAvifHDR(filePath);
}

bool FileDetector::isJpegXlHDR(const QString &filePath)
{
    // JPEG-XL HDR detection using libjxl library
    // This properly decodes the JXL header to extract color encoding information
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QByteArray fileData = file.readAll();
    file.close();
    
    if (fileData.isEmpty()) {
        return false;
    }
    
    // Create JXL decoder
    auto dec = JxlDecoderMake(nullptr);
    if (!dec) {
        return false;
    }
    
    // Subscribe to basic info to get color encoding
    if (JxlDecoderSubscribeEvents(dec.get(), JXL_DEC_BASIC_INFO | JXL_DEC_COLOR_ENCODING) != JXL_DEC_SUCCESS) {
        return false;
    }
    
    // Set input data
    if (JxlDecoderSetInput(dec.get(), 
                           reinterpret_cast<const uint8_t*>(fileData.constData()), 
                           fileData.size()) != JXL_DEC_SUCCESS) {
        return false;
    }
    
    bool isHDR = false;
    JxlBasicInfo info;
    
    // Process decoder events
    while (true) {
        JxlDecoderStatus status = JxlDecoderProcessInput(dec.get());
        
        if (status == JXL_DEC_ERROR) {
            break;
        } else if (status == JXL_DEC_NEED_MORE_INPUT) {
            break;
        } else if (status == JXL_DEC_BASIC_INFO) {
            // Get basic info
            if (JxlDecoderGetBasicInfo(dec.get(), &info) == JXL_DEC_SUCCESS) {
                // Check bits per sample - HDR typically uses > 8 bits
                // However, this alone doesn't determine HDR
                // We need to check the color encoding
            }
        } else if (status == JXL_DEC_COLOR_ENCODING) {
            // Get color encoding
            JxlColorEncoding color_encoding;
            if (JxlDecoderGetColorAsEncodedProfile(dec.get(), JXL_COLOR_PROFILE_TARGET_DATA, &color_encoding) == JXL_DEC_SUCCESS) {
                // Check transfer function
                // PQ (Perceptual Quantizer) = JXL_TRANSFER_FUNCTION_PQ
                // HLG (Hybrid Log-Gamma) = JXL_TRANSFER_FUNCTION_HLG
                if (color_encoding.transfer_function == JXL_TRANSFER_FUNCTION_PQ ||
                    color_encoding.transfer_function == JXL_TRANSFER_FUNCTION_HLG) {
                    isHDR = true;
                }
                
                // Check color primaries
                // BT.2020 = JXL_PRIMARIES_2100
                if (color_encoding.primaries == JXL_PRIMARIES_2100) {
                    isHDR = true;
                }
            }
            break; // We got what we need
        } else if (status == JXL_DEC_SUCCESS) {
            break;
        }
    }
    
    return isHDR;
}

bool FileDetector::isJpegHDR(const QString &filePath)
{
    // JPEG is always SDR
    return false;
}

bool FileDetector::isTiffHDR(const QString &filePath)
{
    // TIFF HDR detection: Look for ICC profile with HDR indicators
    // TIFF files store ICC profiles in tag 34675 (0x8773)
    // We search for "PQ", "Rec. 2020", or "BT.2020" in the profile
    // Note: TIFF files can be very large, so we search in chunks
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    // Read TIFF header (first 8 bytes)
    QByteArray header = file.read(8);
    if (header.size() < 8) {
        file.close();
        return false;
    }
    
    // Check byte order (II = little-endian, MM = big-endian)
    bool littleEndian = (header[0] == 'I' && header[1] == 'I');
    bool bigEndian = (header[0] == 'M' && header[1] == 'M');
    
    if (!littleEndian && !bigEndian) {
        file.close();
        return false;
    }
    
    // Strategy: Read from beginning and end of file to find ICC profile
    // ICC profiles are usually near the end in TIFF files
    
    // Read first 5 MB
    file.seek(0);
    QByteArray dataBegin = file.read(5 * 1024 * 1024);
    QString contentBegin = QString::fromLatin1(dataBegin);
    
    // Check first part
    if (contentBegin.contains("Rec. 2020 PQ", Qt::CaseInsensitive) ||
        contentBegin.contains("BT.2020", Qt::CaseInsensitive)) {
        file.close();
        return true;
    }
    
    bool has2020 = contentBegin.contains("2020", Qt::CaseInsensitive);
    bool hasPQ = contentBegin.contains("PQ", Qt::CaseInsensitive);
    
    if (has2020 && hasPQ) {
        file.close();
        return true;
    }
    
    // Read last 5 MB (where ICC profile usually is)
    qint64 fileSize = file.size();
    if (fileSize > 5 * 1024 * 1024) {
        file.seek(fileSize - 5 * 1024 * 1024);
        QByteArray dataEnd = file.read(5 * 1024 * 1024);
        QString contentEnd = QString::fromLatin1(dataEnd);
        
        if (contentEnd.contains("Rec. 2020 PQ", Qt::CaseInsensitive) ||
            contentEnd.contains("BT.2020", Qt::CaseInsensitive)) {
            file.close();
            return true;
        }
        
        has2020 = contentEnd.contains("2020", Qt::CaseInsensitive);
        hasPQ = contentEnd.contains("PQ", Qt::CaseInsensitive);
        
        if (has2020 && hasPQ) {
            file.close();
            return true;
        }
    }
    
    file.close();
    return false;
}
