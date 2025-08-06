import QtQuick
import de.aaronrust.hdrimageviewer

Window {
    
    id: mainWindow
    
    // Initial state
    visible: false // Will be shown by ImageViewer when first image loads
    width: 1500
    height: 1000
    color: "black"
    
    // Window management properties
    property bool wasMaximized: false
    
    // Window title with loading state
    title: getWindowTitle(App.currentImagePath, imageViewer.isLoading)
    
    // Window management functions
    function toggleFullscreen() {
        if (visibility === Window.FullScreen) {
            exitFullscreen()
        } else {
            enterFullscreen()
        }
    }
    
    function enterFullscreen() {
        // Remember current state before going fullscreen
        wasMaximized = (visibility === Window.Maximized)
        showFullScreen()
    }
    
    function exitFullscreen() {
        if (visibility === Window.FullScreen) {
            if (wasMaximized) {
                showMaximized()
                // Workaround: Second call needed for proper maximized state
                showMaximized()
            } else {
                showNormal()
            }
        }
    }
    
    function moveWindow() {
        // Only allow moving when not in fullscreen
        if (visibility !== Window.FullScreen) {
            startSystemMove()
        }
    }
    
    function isFullscreen() {
        return visibility === Window.FullScreen
    }
    
    function getWindowTitle(imagePath, isLoading) {
        if (!imagePath) {
            return i18n("HDR Image Viewer")
        }
        
        let path = imagePath.toString()
        
        // Remove file:// prefix if present
        if (path.startsWith("file://")) {
            path = path.substring(7)
        }
        
        const fileName = path.split('/').pop()
        const loadingText = isLoading ? " " + i18n("(loading...)") : ""
        
        return fileName + loadingText + " - " + i18n("HDR Image Viewer")
    }
    
    ImageViewer {
        id: imageViewer
        height: parent.height
        width: parent.width
        parentWindow: mainWindow
        
        source: App.currentImagePath || imagePath
        fallbackSource: imageViewer.lastImagePath
        
        onStartWindowMove: {
            mainWindow.moveWindow()
        }
        
        onDoubleClicked: {
            mainWindow.toggleFullscreen()
        }
        
    }   

}