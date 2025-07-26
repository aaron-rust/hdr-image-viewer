import QtQuick
import QtQuick.Controls as QQC2

Item {
    id: root
    
    // Public properties
    property alias source: mainImage.source
    property alias status: mainImage.status
    property bool isLoading: mainImage.status === Image.Loading
    property string fallbackSource: ""
    
    // Zoom configuration
    property real zoomFactor: 1.0
    property real minZoom: 1.0
    property real maxZoom: 100.0
    property real zoomStep: 1.02
    
    // Signals
    signal clicked()
    signal doubleClicked()
    signal imageReady()
    signal imageError()
    signal startWindowMove()
    
    // Public methods
    function resetZoom() {
        zoomFactor = 1.0
        imageFlickable.contentX = 0
        imageFlickable.contentY = 0
    }
    
    function zoomIn(centerX, centerY) {
        _performZoom(zoomStep, centerX, centerY)
    }
    
    function zoomOut(centerX, centerY) {
        _performZoom(1.0 / zoomStep, centerX, centerY)
    }
    
    function fitToWindow() {
        resetZoom()
    }
    
    // Private methods
    function _performZoom(factor, centerX, centerY) {
        const oldZoom = zoomFactor
        const newZoom = Math.max(minZoom, Math.min(maxZoom, zoomFactor * factor))
        
        if (Math.abs(newZoom - oldZoom) < 0.001) {
            return // No significant change
        }
        
        zoomFactor = newZoom
        
        if (zoomFactor <= minZoom) {
            // Reset to center when at minimum zoom
            imageFlickable.contentX = 0
            imageFlickable.contentY = 0
        } else {
            // Zoom towards the specified point
            const ratioChange = newZoom / oldZoom
            const contentCenterX = imageFlickable.contentX + (centerX || width / 2)
            const contentCenterY = imageFlickable.contentY + (centerY || height / 2)
            
            imageFlickable.contentX = Math.max(0, Math.min(
                contentCenterX * ratioChange - (centerX || width / 2),
                imageFlickable.contentWidth - imageFlickable.width))
            imageFlickable.contentY = Math.max(0, Math.min(
                contentCenterY * ratioChange - (centerY || height / 2),
                imageFlickable.contentHeight - imageFlickable.height))
        }
    }
    
    // Main container
    Rectangle {
        anchors.fill: parent
        color: "black"
        
        Flickable {
            id: imageFlickable
            anchors.fill: parent
            contentWidth: imageContainer.width
            contentHeight: imageContainer.height
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            interactive: zoomFactor > 1.0
            
            Item {
                id: imageContainer
                width: Math.max(mainImage.width * zoomFactor, imageFlickable.width)
                height: Math.max(mainImage.height * zoomFactor, imageFlickable.height)
                
                // Fallback image (shown during loading)
                Image {
                    id: fallbackImage
                    anchors.centerIn: parent
                    width: imageFlickable.width
                    height: imageFlickable.height
                    fillMode: Image.PreserveAspectFit
                    source: root.fallbackSource
                    visible: root.isLoading && root.fallbackSource !== ""
                    smooth: true
                    mipmap: true
                    cache: true
                    asynchronous: false
                    
                    transform: Scale {
                        xScale: root.zoomFactor
                        yScale: root.zoomFactor
                        origin.x: fallbackImage.width / 2
                        origin.y: fallbackImage.height / 2
                    }
                }
                
                // Main image
                Image {
                    id: mainImage
                    anchors.centerIn: parent
                    width: imageFlickable.width
                    height: imageFlickable.height
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: true
                    cache: true
                    asynchronous: true
                    
                    transform: Scale {
                        xScale: root.zoomFactor
                        yScale: root.zoomFactor
                        origin.x: mainImage.width / 2
                        origin.y: mainImage.height / 2
                    }
                    
                    onStatusChanged: {
                        if (status === Image.Ready) {
                            root.imageReady()
                        } else if (status === Image.Error) {
                            root.imageError()
                        }
                    }
                }
            }
        }
        
        // Mouse interaction handling
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            
            property bool isDragging: false
            property real dragStartX: 0
            property real dragStartY: 0
            property real contentStartX: 0
            property real contentStartY: 0
            
            onPressed: (mouse) => {
                if (mouse.button === Qt.LeftButton) {
                    if (root.zoomFactor > 1.0) {
                        isDragging = true
                        dragStartX = mouse.x
                        dragStartY = mouse.y
                        contentStartX = imageFlickable.contentX
                        contentStartY = imageFlickable.contentY
                    } else {
                        // When not zoomed, start window dragging
                        root.startWindowMove()
                    }
                }
            }
            
            onPositionChanged: (mouse) => {
                if (isDragging && root.zoomFactor > 1.0) {
                    const deltaX = mouse.x - dragStartX
                    const deltaY = mouse.y - dragStartY
                    
                    imageFlickable.contentX = Math.max(0, Math.min(
                        contentStartX - deltaX,
                        imageFlickable.contentWidth - imageFlickable.width))
                    imageFlickable.contentY = Math.max(0, Math.min(
                        contentStartY - deltaY,
                        imageFlickable.contentHeight - imageFlickable.height))
                }
            }
            
            onReleased: () => {
                isDragging = false
            }
            
            onClicked: root.clicked()
            onDoubleClicked: root.doubleClicked()
            
            onWheel: (wheel) => {
                const angleDelta = wheel.angleDelta.y
                if (angleDelta > 0) {
                    root.zoomIn(wheel.x, wheel.y)
                } else if (angleDelta < 0) {
                    root.zoomOut(wheel.x, wheel.y)
                }
                wheel.accepted = true
            }
        }
    }
    
    // Loading indicator
    QQC2.BusyIndicator {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 20
        anchors.bottomMargin: 20
        visible: root.isLoading
        running: visible
    }
}
