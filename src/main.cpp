#include <QtGlobal>
#include <QApplication>

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QUrl>

#include "app.h"
#include "file_detector.h"
#include "version-hdr-image-viewer.h"
#include <KAboutData>
#include <KLocalizedContext>
#include <KLocalizedString>

using namespace Qt::Literals::StringLiterals;

namespace {
    constexpr int SUCCESS = 0;
    constexpr int INVALID_ARGS = 1;
    constexpr int FILE_NOT_FOUND = 2;
    constexpr int UNSUPPORTED_FORMAT = 3;
    constexpr int ENGINE_FAILED = 4;
    
    QString processImagePath(const QString &filePath) {
        QFileInfo fileInfo(filePath);
        
        if (!fileInfo.exists()) {
            qCritical() << "Error: Image file does not exist:" << filePath;
            return {};
        }
        
        if (fileInfo.isRelative()) {
            return QUrl::fromLocalFile(QDir::current().absoluteFilePath(filePath)).toString();
        } else {
            return QUrl::fromLocalFile(filePath).toString();
        }
    }
    
    void setupApplication() {
        // Optimize for large HDR images (8GB allocation limit)
        qputenv("QT_IMAGEIO_MAXALLOC", "8192");

        // Use KDE desktop style unless overridden
        if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
            QQuickStyle::setStyle(u"org.kde.desktop"_s);
        }

        KLocalizedString::setApplicationDomain("hdr-image-viewer");
        QCoreApplication::setOrganizationName(u"KDE"_s);
    }
    
    void setupAboutData() {
        KAboutData aboutData(
            u"hdr-image-viewer"_s,
            i18nc("@title", "HDR Image Viewer"),
            QStringLiteral(HDR_IMAGE_VIEWER_VERSION_STRING),
            i18n("HDR Image Viewer with Color Management"),
            KAboutLicense::MIT,
            i18n("(c) 2025 Aaron Rust"));
            
        aboutData.addAuthor(
            i18nc("@info:credit", "Aaron Rust"), 
            i18nc("@info:credit", "Maintainer & Developer"));
            
        KAboutData::setApplicationData(aboutData);
        QGuiApplication::setWindowIcon(QIcon::fromTheme(u"de.aaronrust.hdr-image-viewer"_s));
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    setupApplication();
    setupAboutData();

    QCommandLineParser parser;
    parser.setApplicationDescription(i18n("HDR Image Viewer with advanced color management"));

    // Essential command line options
    QCommandLineOption helpOption({u"h"_s, u"help"_s}, i18n("Display help information"));
    parser.addOption(helpOption);
    parser.addVersionOption();
    
    parser.addPositionalArgument(u"image"_s, i18n("Image file to display"));
    parser.process(app);

    if (parser.isSet(helpOption)) {
        parser.showHelp();
        return SUCCESS;
    }

    // Validate arguments
    const QStringList args = parser.positionalArguments();
    if (args.isEmpty()) {
        qCritical() << "Error: No image file specified.";
        qCritical() << "Usage:" << QCoreApplication::applicationName() << "<image-file>";
        return INVALID_ARGS;
    }
    
    // Process image path
    QString imagePath = processImagePath(args.first());
    if (imagePath.isEmpty()) {
        return FILE_NOT_FOUND;
    }
    
    // Check if the image format is supported
    QString localPath = QUrl(imagePath).toLocalFile();
    if (!FileDetector::isSupportedImageFormat(localPath)) {
        qCritical() << "Error: Unsupported image format:" << localPath;
        return UNSUPPORTED_FORMAT;
    }

    // Setup QML engine
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.rootContext()->setContextProperty(u"imagePath"_s, imagePath);

    engine.loadFromModule("de.aaronrust.hdrimageviewer", u"Main");

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load QML application";
        return ENGINE_FAILED;
    }

    return app.exec();
}
