#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QStringList>
#include <QUrl>

#include <memory>
#include <optional>
#include <unordered_map>

#include "color_management.h"

class ImageNavigator : public QObject
{
    Q_OBJECT

public:
    explicit ImageNavigator(QObject *parent = nullptr);

    void initializeFromPath(const QString &imagePath);
    void navigateNext();
    void navigatePrevious();
    
    QString currentImagePath() const { return m_currentImagePath; }
    int currentIndex() const { return m_currentIndex; }
    int totalImages() const { return m_imageList.size(); }

Q_SIGNALS:
    void currentImageChanged(const QString &imagePath);

private:
    void loadImageListFromDirectory(const QString &currentImagePath);
    static QStringList getSupportedImageExtensions();

    QStringList m_imageList;
    QString m_currentImagePath;
    int m_currentIndex = -1;
};

class ColorController : public QObject
{
    Q_OBJECT

public:
    explicit ColorController(QObject *parent = nullptr);
    ~ColorController() override;

    void setupWindow(QQuickWindow *window);
    void setPQMode(QQuickWindow *window, int referenceLuminance);
    void setColorMode(QQuickWindow *window, ColorManagementSurface::ColorMode mode);
    
    QString getPreferredDescription(QQuickWindow *window) const;

Q_SIGNALS:
    void preferredDescriptionChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void createSurfaceForWindow(QQuickWindow *window);

    struct WindowData {
        std::optional<ColorManagementSurface::ColorMode> colorMode;
        int referenceLuminance = 100;
    };

    std::unique_ptr<ColorManagementGlobal> m_global;
    std::unordered_map<QQuickWindow*, WindowData> m_windowData;
    std::unordered_map<QQuickWindow*, std::unique_ptr<ColorManagementSurface>> m_surfaces;
};

class App : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString currentImagePath READ currentImagePath NOTIFY currentImagePathChanged)
    Q_PROPERTY(QString preferredDescription READ preferredDescription NOTIFY preferredDescriptionChanged)

public:
    explicit App(QObject *parent = nullptr);
    ~App() override = default;

    // Window management
    Q_INVOKABLE void setupMainWindow(QQuickWindow *window);
    Q_INVOKABLE void adjustWindowSizeToImage(QQuickWindow *window, const QString &imagePath);
    
    // Color management
    Q_INVOKABLE void enablePQMode(QQuickWindow *window, int referenceLuminance = 203);
    Q_INVOKABLE void setColorProfile(QQuickWindow *window, int profileId);
    
    // Image navigation
    Q_INVOKABLE void initializeImageList(const QString &imagePath);
    Q_INVOKABLE void navigateToNext();
    Q_INVOKABLE void navigateToPrevious();

    // Properties
    QString currentImagePath() const;
    QString preferredDescription() const;

Q_SIGNALS:
    void currentImagePathChanged();
    void preferredDescriptionChanged();

private:
    void connectSignals();

    std::unique_ptr<ImageNavigator> m_imageNavigator;
    std::unique_ptr<ColorController> m_colorController;
    QQuickWindow *m_mainWindow = nullptr;
};