import QtQuick

QtObject {
    id: root
    
    property Window window: null
    property bool wasMaximized: false
    
    function toggleFullscreen() {
        if (!window) return
        
        if (window.visibility === Window.FullScreen) {
            exitFullscreen()
        } else {
            enterFullscreen()
        }
    }
    
    function enterFullscreen() {
        if (!window) return
        
        // Remember current state before going fullscreen
        wasMaximized = (window.visibility === Window.Maximized)
        window.showFullScreen()
    }
    
    function exitFullscreen() {
        if (!window) return
        
        if (window.visibility === Window.FullScreen) {
            if (wasMaximized) {
                window.showMaximized()
                // Workaround: Second call needed for proper maximized state
                window.showMaximized()
            } else {
                window.showNormal()
            }
        }
    }
    
    function moveWindow() {
        if (!window) return
        
        // Only allow moving when not in fullscreen
        if (window.visibility !== Window.FullScreen) {
            window.startSystemMove()
        }
    }
    
    function isFullscreen() {
        return window ? window.visibility === Window.FullScreen : false
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
}
