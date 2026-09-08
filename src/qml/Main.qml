import QtQuick
import de.aaronrust.hdrimageviewer

Window {

    id: mainWindow

    visible: false
    width: 1500
    height: 1000
    color: "black"

    property int preFsWidth: 0
    property int preFsHeight: 0

    title: getWindowTitle(App.currentImagePath, imageViewer.isLoading, imageViewer.isHDRMode)

    function toggleFullscreen() {
        if (visibility === Window.FullScreen)
            exitFullscreen()
        else
            enterFullscreen()
    }

    function enterFullscreen() {
        preFsWidth = width
        preFsHeight = height
        showFullScreen()
    }

    function exitFullscreen() {
        if (visibility !== Window.FullScreen)
            return
        App.exitFullscreen(mainWindow, preFsWidth, preFsHeight)
    }

    function moveWindow() {
        if (visibility !== Window.FullScreen)
            startSystemMove()
    }

    function getWindowTitle(imagePath, isLoading, isHDR) {
        if (!imagePath)
            return i18n("HDR Image Viewer")

        let path = imagePath.toString()
        if (path.startsWith("file://"))
            path = path.substring(7)

        const fileName = path.split('/').pop()
        const loadingText = isLoading ? " " + i18n("(loading...)") : ""
        const modeText = isHDR ? "HDR" : "SDR"

        return fileName + loadingText + " – " + "color mode: " + modeText + " – " + i18n("HDR Image Viewer")
    }

    ImageViewer {
        id: imageViewer
        height: parent.height
        width: parent.width
        parentWindow: mainWindow
        source: App.currentImagePath || imagePath

        onStartWindowMove: mainWindow.moveWindow()
        onDoubleClicked: mainWindow.toggleFullscreen()
    }
}
