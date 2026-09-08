#include "app.h"
#include "file_detector.h"

#include <QCursor>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImageReader>
#include <QMimeDatabase>
#include <QPlatformSurfaceEvent>
#include <QQuickWindow>
#include <QScreen>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <qpa/qplatformwindow_p.h>

namespace {
constexpr int DEFAULT_PQ_REFERENCE_LUMINANCE = 203;
}

ImageNavigator::ImageNavigator(QObject *parent) : QObject(parent) {}

void ImageNavigator::initializeFromPath(const QString &imagePath) {
  loadImageListFromDirectory(imagePath);
}

void ImageNavigator::navigateNext() {
  if (m_imageList.isEmpty() || m_currentIndex < 0) {
    return;
  }

  m_currentIndex = (m_currentIndex + 1) % m_imageList.size();
  m_currentImagePath = m_imageList[m_currentIndex];
  Q_EMIT currentImageChanged(m_currentImagePath);
}

void ImageNavigator::navigatePrevious() {
  if (m_imageList.isEmpty() || m_currentIndex < 0) {
    return;
  }

  m_currentIndex =
      (m_currentIndex - 1 + m_imageList.size()) % m_imageList.size();
  m_currentImagePath = m_imageList[m_currentIndex];
  Q_EMIT currentImageChanged(m_currentImagePath);
}

void ImageNavigator::loadImageListFromDirectory(
    const QString &currentImagePath) {
  QString localPath = currentImagePath;
  if (localPath.startsWith(QStringLiteral("file://"))) {
    localPath = QUrl(localPath).toLocalFile();
  }

  QFileInfo currentFile(localPath);
  if (!currentFile.exists()) {
    return;
  }

  QDir directory = currentFile.dir();

  // Get all files sorted by name
  QStringList allFiles = directory.entryList(QDir::Files, QDir::Name);

  m_imageList.clear();
  for (const QString &fileName : allFiles) {
    QString fullPath = directory.absoluteFilePath(fileName);

    if (FileDetector::isSupportedImageFormat(fullPath)) {
      m_imageList.append(QUrl::fromLocalFile(fullPath).toString());
    }
  }

  // Find current file index
  QString currentUrl = QUrl::fromLocalFile(localPath).toString();
  m_currentIndex = m_imageList.indexOf(currentUrl);

  if (m_currentIndex >= 0) {
    m_currentImagePath = currentUrl;
  } else {
    // Fallback: use provided path as current
    m_currentImagePath = currentImagePath;
    m_currentIndex = 0;
  }

  Q_EMIT currentImageChanged(m_currentImagePath);
}

ColorController::ColorController(QObject *parent)
    : QObject(parent), m_global(std::make_unique<ColorManagementGlobal>()) {}

ColorController::~ColorController() = default;

void ColorController::setupWindow(QQuickWindow *window) {
  // Initialize with default PQ mode for HDR
  setPQMode(window, DEFAULT_PQ_REFERENCE_LUMINANCE);
}

void ColorController::setPQMode(QQuickWindow *window, int referenceLuminance) {
  m_windowData[window] = {
      .colorMode = std::nullopt,
      .referenceLuminance = referenceLuminance,
  };
  createSurfaceForWindow(window);
}

void ColorController::setColorMode(QQuickWindow *window,
                                   ColorManagementSurface::ColorMode mode) {
  m_windowData[window] = {
      .colorMode = mode,
      .referenceLuminance = 100,
  };
  createSurfaceForWindow(window);
}

QString ColorController::getPreferredDescription(QQuickWindow *window) const {
  const auto it = m_surfaces.find(window);
  if (it != m_surfaces.end() && it->second->feedback()->preferred()) {
    return it->second->feedback()->preferred()->info().description();
  }
  return QStringLiteral("Display capabilities unknown");
}

bool ColorController::eventFilter(QObject *watched, QEvent *event) {
  auto window = qobject_cast<QQuickWindow *>(watched);
  if (!window || event->type() != QEvent::PlatformSurface) {
    return false;
  }

  auto surfaceEvent = static_cast<QPlatformSurfaceEvent *>(event);
  if (surfaceEvent->surfaceEventType() ==
      QPlatformSurfaceEvent::SurfaceCreated) {
    createSurfaceForWindow(window);
  }

  return false;
}

void ColorController::createSurfaceForWindow(QQuickWindow *window) {
  const auto it = m_windowData.find(window);
  if (it == m_windowData.end()) {
    return;
  }

  auto waylandWindow =
      window->nativeInterface<QNativeInterface::Private::QWaylandWindow>();
  if (!waylandWindow) {
    window->installEventFilter(this);
    return;
  }

  auto &surface = m_surfaces[window];
  if (!surface) {
    auto feedback = std::make_unique<ColorManagementFeedback>(
        m_global->get_surface_feedback(waylandWindow->surface()));
    connect(feedback.get(), &ColorManagementFeedback::preferredChanged, this,
            &ColorController::preferredDescriptionChanged);

    surface = std::make_unique<ColorManagementSurface>(
        m_global.get(), window, m_global->get_surface(waylandWindow->surface()),
        std::move(feedback));
  }

  const auto &data = it->second;
  if (data.colorMode.has_value()) {
    surface->setColorMode(*data.colorMode);
  } else {
    surface->setPQMode(data.referenceLuminance);
  }
}

App::App(QObject *parent)
    : QObject(parent), m_imageNavigator(std::make_unique<ImageNavigator>(this)),
      m_colorController(std::make_unique<ColorController>(this)) {
  connectSignals();
}

void App::setupMainWindow(QQuickWindow *window) {
  m_mainWindow = window;
  m_colorController->setupWindow(window);
  window->installEventFilter(this);
}

void App::adjustWindowSizeToImage(QQuickWindow *window,
                                  const QString &imagePath) {
  if (!window) {
    return;
  }

  // Convert URL to local file path if needed
  QString localPath = imagePath;
  if (localPath.startsWith(QStringLiteral("file://"))) {
    localPath = QUrl(localPath).toLocalFile();
  }

  // Read image dimensions
  QImageReader reader(localPath);
  if (!reader.canRead()) {
    qWarning() << "Cannot read image for size adjustment:" << localPath;
    return;
  }

  QSize imageSize = reader.size();
  if (!imageSize.isValid() || imageSize.width() <= 0 ||
      imageSize.height() <= 0) {
    qWarning() << "Invalid image size:" << imageSize;
    return;
  }

  // Get screen geometry
  QScreen *screen = window->screen();
  if (!screen) {
    screen = QGuiApplication::primaryScreen();
  }

  if (!screen) {
    qWarning() << "No screen available for window size adjustment";
    return;
  }

  QRect screenGeometry = screen->availableGeometry();

  // Calculate target height (3/4 of screen height)
  int targetHeight = (screenGeometry.height() * 3) / 4;

  // Calculate width based on image aspect ratio
  double aspectRatio =
      static_cast<double>(imageSize.width()) / imageSize.height();
  int targetWidth = static_cast<int>(targetHeight * aspectRatio);

  // Ensure window doesn't exceed screen width (leave 10% margin)
  if (targetWidth > screenGeometry.width() * 0.9) {
    targetWidth = static_cast<int>(screenGeometry.width() * 0.9);
    targetHeight = static_cast<int>(targetWidth / aspectRatio);
  }

  // Apply minimum size constraints (at least 400x300)
  const int minWidth = 400;
  const int minHeight = 300;

  if (targetWidth < minWidth) {
    targetWidth = minWidth;
    targetHeight = static_cast<int>(targetWidth / aspectRatio);
  }

  if (targetHeight < minHeight) {
    targetHeight = minHeight;
    targetWidth = static_cast<int>(targetHeight * aspectRatio);
  }

  // Apply new size
  window->setWidth(targetWidth);
  window->setHeight(targetHeight);
}

void App::exitFullscreen(QQuickWindow *window, int width, int height) {
  if (!window)
    return;

  // Phase 1: clear only the fullscreen bit so xdg_toplevel.unset_fullscreen
  // is committed on its own. Clearing maximized at the same time would put
  // both unset requests in the same commit, which triggers a state-inversion
  // bug on Hyprland.
  window->setWindowStates(window->windowStates() & ~Qt::WindowFullScreen);

  // Phase 2: on the next event-loop iteration (after the compositor has
  // re-tiled and ACKed), send the size hint + unset_maximized together.
  QTimer::singleShot(0, window, [window, width, height] {
    window->resize(width, height);
    window->setWindowStates(window->windowStates() & ~Qt::WindowMaximized);
  });
}

void App::enablePQMode(QQuickWindow *window, int referenceLuminance) {
  m_colorController->setPQMode(window, referenceLuminance);
}

void App::disablePQMode(QQuickWindow *window) {
  // Set to sRGB mode for SDR images
  m_colorController->setColorMode(window,
                                  ColorManagementSurface::ColorMode::Default);
}

void App::setColorProfile(QQuickWindow *window, int profileId) {
  auto mode = static_cast<ColorManagementSurface::ColorMode>(profileId);
  m_colorController->setColorMode(window, mode);
}

bool App::isImageHDR(const QString &imagePath) {
  return FileDetector::isImageHDR(imagePath);
}

void App::setCursorHidden(QQuickWindow *window, bool hidden) {
  if (!window) {
    return;
  }

  const auto it = m_cursorHidden.find(window);
  const bool wasHidden = (it != m_cursorHidden.end()) ? it->second : false;
  if (wasHidden == hidden) {
    return;
  }

  window->setCursor(QCursor(hidden ? Qt::BlankCursor : Qt::ArrowCursor));
  m_cursorHidden[window] = hidden;
}

void App::initializeImageList(const QString &imagePath) {
  m_imageNavigator->initializeFromPath(imagePath);
}

void App::navigateToNext() { m_imageNavigator->navigateNext(); }

void App::navigateToPrevious() { m_imageNavigator->navigatePrevious(); }

QString App::currentImagePath() const {
  return m_imageNavigator->currentImagePath();
}

QString App::preferredDescription() const {
  if (m_mainWindow) {
    return m_colorController->getPreferredDescription(m_mainWindow);
  }
  return QStringLiteral("No window available");
}

void App::connectSignals() {
  connect(m_imageNavigator.get(), &ImageNavigator::currentImageChanged, this,
          &App::currentImagePathChanged);
  connect(m_colorController.get(),
          &ColorController::preferredDescriptionChanged, this,
          &App::preferredDescriptionChanged);

  // Clear the fullscreen state and flush to the compositor before the
  // event loop exits.
  connect(qApp, &QCoreApplication::aboutToQuit, this, [this] {
    if (!m_mainWindow)
      return;
    auto states = m_mainWindow->windowStates();
    if (states & Qt::WindowFullScreen) {
      m_mainWindow->setWindowStates(states & ~Qt::WindowFullScreen);
      QCoreApplication::processEvents();
    }
  });
}

bool App::eventFilter(QObject *watched, QEvent *event) {
  if (watched != m_mainWindow || !m_mainWindow)
    return false;

  if (event->type() == QEvent::Close &&
      (m_mainWindow->windowStates() & Qt::WindowFullScreen)) {
    m_mainWindow->setWindowStates(m_mainWindow->windowStates() &
                                  ~Qt::WindowFullScreen);
  }
  return false;
}

#include "moc_app.cpp"
