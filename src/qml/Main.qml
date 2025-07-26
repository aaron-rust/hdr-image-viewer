import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import de.aaronrust.hdrimageviewer

Window {
    id: mainWindow
    
    // Initial state
    visible: false  // Show when first image loads
    width: 1500
    height: 1000
    color: "black"
    
    // State management
    property bool isFirstLoad: true
    property string lastImagePath: ""
    
    // Window title with loading state
    title: windowManager.getWindowTitle(App.currentImagePath, imageViewer.isLoading)
    
    // Window management
    WindowManager {
        id: windowManager
        window: mainWindow
    }
    
    // Keyboard shortcuts
    KeyboardHandler {
        id: keyboardHandler
        
        onToggleFullscreen: windowManager.toggleFullscreen()
        onExitFullscreen: {
            if (windowManager.isFullscreen()) {
                windowManager.exitFullscreen()
            }
        }
        onQuitApplication: Qt.quit()
        onNavigateNext: App.navigateToNext()
        onNavigatePrevious: App.navigateToPrevious()
        onZoomIn: imageViewer.zoomIn()
        onZoomOut: imageViewer.zoomOut()
        onResetZoom: imageViewer.resetZoom()
    }
    
    // Main image viewer
    ImageViewer {
        id: imageViewer
        anchors.fill: parent
        
        source: App.currentImagePath || imagePath
        fallbackSource: lastImagePath
        
        onClicked: {
            // Single click behavior - kept for compatibility
        }
        
        onStartWindowMove: {
            windowManager.moveWindow()
        }
        
        onDoubleClicked: {
            windowManager.toggleFullscreen()
        }
        
        onImageReady: {
            lastImagePath = App.currentImagePath
            
            // Adjust window size on first load
            if (isFirstLoad) {
                App.adjustWindowSizeToImage(mainWindow, App.currentImagePath || imagePath)
                mainWindow.visible = true
                isFirstLoad = false
            }
        }
        
        onImageError: {
            // Show window even if first image fails
            if (isFirstLoad) {
                mainWindow.visible = true
                isFirstLoad = false
            }
        }
    }
    
    // Application initialization
    Component.onCompleted: {
        // Setup HDR color management
        App.setupMainWindow(mainWindow)
        App.enablePQMode(mainWindow, 203)
        
        // Initialize image navigation
        App.initializeImageList(imagePath)
    }
    
    // Handle window state changes for proper cleanup
    onVisibilityChanged: {
        // Ensure keyboard focus is maintained
        keyboardHandler.forceActiveFocus()
    }
}

