import QtQuick
import QtQuick.Controls as QQC2
import de.aaronrust.hdrimageviewer

Item {
    id: root
    
    // Public properties
    // Externally set image source
    property url source: ""
    // Status / loading flags of the currently visible image
    property int status: mainImageA.status
    property bool isLoading: mainImageA.status === Image.Loading
    property bool isHDRMode: currentHDRMode  // Expose current HDR mode state

    // State management
    property bool isFirstLoad: true
    property string lastImagePath: ""

    // React to changed external source and start loading
    onSourceChanged: {
        print("Source changed to:", source)
        if (!source) return
        loadNewImage(source)
    }

    // Single image logic: retainWhileLoading keeps the previous frame visible
    function loadNewImage(newSource) {
        print("Loading new image:", newSource)
        mainImageA.source = newSource
    }
    
    // Signals
    signal clicked()
    signal doubleClicked()
    signal imageReady()
    signal imageError()
    signal startWindowMove()
    
    // Internal properties for fullscreen communication
    property var parentWindow: null
    
    // Window visibility management
    function showParentWindow() {
        if (parentWindow && !parentWindow.visible) {
            parentWindow.visible = true
        }
    }
    
    // Fullscreen functions
    function toggleFullscreen() {
        if (parentWindow) {
            if (parentWindow.visibility === Window.FullScreen) {
                exitFullscreen()
            } else {
                enterFullscreen()
            }
        }
    }
    
    function enterFullscreen() {
        if (parentWindow) {
            // Remember current state before going fullscreen
            parentWindow.wasMaximized = (parentWindow.visibility === Window.Maximized)
            parentWindow.showFullScreen()
        }
    }
    
    function exitFullscreen() {
        if (parentWindow && parentWindow.visibility === Window.FullScreen) {
            if (parentWindow.wasMaximized) {
                parentWindow.showMaximized()
                // Workaround: Second call needed for proper maximized state
                parentWindow.showMaximized()
            } else {
                parentWindow.showNormal()
            }
        }
    }

    // Keyboard handling properties
    property bool wPressed: false
    property bool aPressed: false
    property bool sPressed: false
    property bool dPressed: false
    property bool qPressed: false
    property bool ePressed: false
    
    // HDR mode toggle - track current state
    property bool currentHDRMode: false
    
    // Smooth rendering toggle for pixel art mode
    property bool smoothRendering: true
    
    function toggleHDRMode() {
        // Simply invert the current state
        if (currentHDRMode) {
            App.disablePQMode(hdrWindow)
            currentHDRMode = false
            print("HDR mode manually disabled")
        } else {
            App.enablePQMode(hdrWindow)
            currentHDRMode = true
            print("HDR mode manually enabled")
        }
    }

    // Zoom functionality
    property real zoomFactor: 1.0
    property real minZoom: 1.0
    property real maxZoom: 100.0
    property real zoomStep: 1.03
    property real _zoomCenterX: width / 2
    property real _zoomCenterY: height / 2
    property real smoothZoomVelocity: 0.0
    function resetZoom() {
        zoomFactor = 1.0
        smoothZoomVelocity = 0.0
        smoothZoomAnimation.stop()
        imageFlickable.contentX = 0
        imageFlickable.contentY = 0
    }
    function zoomInKeyboard() {
        triggerZoom(0.02, width / 2, height / 2)
    }
    function zoomOutKeyboard() {
        triggerZoom(-0.02, width / 2, height / 2)
    }
    function fitToWindow() {
        resetZoom()
    }
    function triggerZoom(stepSizeFactor, centerX, centerY) {
        _zoomCenterX = centerX || width / 2
        _zoomCenterY = centerY || height / 2
        
        smoothZoomVelocity += stepSizeFactor

        if (!smoothZoomAnimation.running) {
            smoothZoomAnimation.start()
        }
    }
    FrameAnimation {
        id: smoothZoomAnimation
        running: false
        
        onTriggered: {
            // Stop if velocity is too small
            if (Math.abs(root.smoothZoomVelocity) < 0.01) {
                running = false
                return
            }
            
            // Apply zoom based on velocity
            const zoomFactor = root.smoothZoomVelocity > 0 ? root.zoomStep : 1.0 / root.zoomStep
            const steps = Math.abs(root.smoothZoomVelocity)  // Direct use of velocity as step count
            for (let i = 0; i < Math.ceil(steps); i++) {
                root.performZoomStep(zoomFactor, root._zoomCenterX, root._zoomCenterY)
            }
            
            // Slow down velocity
            root.smoothZoomVelocity *= 0.5
        }
    }
    function performZoomStep(factor, centerX, centerY) {
        const oldZoom = zoomFactor
        const newZoom = Math.max(minZoom, Math.min(maxZoom, zoomFactor * factor))
        
        if (Math.abs(newZoom - oldZoom) < 0.001) {
            return
        }
        
        zoomFactor = newZoom
        
        if (zoomFactor <= minZoom + 0.001) {
            // At or very close to minimum zoom - fully reset
            resetZoom()
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
    // Zoom functionality END

    // Movement functionality
    function moveUp() {
        if (root.zoomFactor > 1.0) {
            triggerMove(0, -1)
        }
    }
    function moveDown() {
        if (root.zoomFactor > 1.0) {
            triggerMove(0, 1)
        }
    }
    function moveLeft() {
        if (root.zoomFactor > 1.0) {
            triggerMove(-1, 0)
        }
    }
    function moveRight() {
        if (root.zoomFactor > 1.0) {
            triggerMove(1, 0)
        }
    }
    function stopMovement() {}
    function startMovement() {}
    function triggerMove(xDirection, yDirection) {
        const moveStep = 4 // Optimized for ~240Hz (4 * 240 = 960 pixels/sec)
        if (root.zoomFactor > 1.0) {
            const newX = Math.max(0, Math.min(
                imageFlickable.contentX + (xDirection * moveStep),
                imageFlickable.contentWidth - imageFlickable.width))
            const newY = Math.max(0, Math.min(
                imageFlickable.contentY + (yDirection * moveStep),
                imageFlickable.contentHeight - imageFlickable.height))
                
            imageFlickable.contentX = newX
            imageFlickable.contentY = newY
        }
    }
    // Movement functionality END

    // Movement timer for smooth keyboard navigation
    Timer {
        id: movementTimer
        interval: 4 // Optimized for ~240Hz (4 * 240 = 960 pixels/sec)
        repeat: true
        running: root.wPressed || root.aPressed || root.sPressed || root.dPressed
        
        property int frameCount: 0
        
        onTriggered: {
            frameCount++            
            if (root.wPressed) root.moveUp()
            if (root.sPressed) root.moveDown()
            if (root.aPressed) root.moveLeft()
            if (root.dPressed) root.moveRight()
        }
        
        onRunningChanged: {
            if (!running) {
                frameCount = 0
                root.stopMovement()
            } else {
                frameCount = 0
            }
        }
    }

    FrameAnimation {
        id: zoomAnimation
        running: root.qPressed || root.ePressed
        
        onTriggered: {
            if (root.qPressed) root.zoomOutKeyboard()
            if (root.ePressed) root.zoomInKeyboard()
        }
    }

    // HDR Window Container
    WindowContainer {
        anchors.fill: parent
        
        window: Window {
            id: hdrWindow
            
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
                    interactive: root.zoomFactor > 1.0
                    
                    Item {
                        id: imageContainer
                        width: Math.max(mainImageA.paintedWidth * root.zoomFactor, imageFlickable.width)
                        height: Math.max(mainImageA.paintedHeight * root.zoomFactor, imageFlickable.height)

                        // Image A
                        Image {
                            id: mainImageA
                            anchors.centerIn: parent
                            width: imageFlickable.width
                            height: imageFlickable.height
                            fillMode: Image.PreserveAspectFit
                            smooth: root.smoothRendering
                            mipmap: true
                            cache: false
                            asynchronous: false
                            retainWhileLoading: true
                            autoTransform: true
                            
                            transform: Scale {
                                xScale: root.zoomFactor
                                yScale: root.zoomFactor
                                origin.x: mainImageA.width / 2
                                origin.y: mainImageA.height / 2
                            }
                            onStatusChanged: {
                                if (mainImageA.status === Image.Ready) {
                                    const newSource = mainImageA.source
                                    root.lastImagePath = newSource

                                    print("Image loaded:", newSource)

                                    if (App.isImageHDR(newSource)) {
                                        print("Detected as HDR - enabling PQ mode")
                                        App.enablePQMode(hdrWindow)
                                        root.currentHDRMode = true
                                    } else {
                                        print("Detected as SDR - disabling PQ mode")
                                        App.disablePQMode(hdrWindow)
                                        root.currentHDRMode = false
                                    }

                                    if (root.isFirstLoad) {
                                        root.isFirstLoad = false
                                        root.showParentWindow()
                                    }

                                    root.imageReady()
                                } else if (mainImageA.status === Image.Error) {
                                    if (root.isFirstLoad) {
                                        root.isFirstLoad = false
                                        root.showParentWindow()
                                    }
                                    root.imageError()
                                }
                            }

                        }
                    }
                }
                
                // Mouse interaction handling
                MouseArea {
                    id: imageMouseArea
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    hoverEnabled: true

                    property bool cursorHidden: false

                    function showCursorAndArmHideTimer() {
                        if (cursorHidden) {
                            cursorHidden = false
                            if (App && App["setCursorHidden"]) App["setCursorHidden"](hdrWindow, false)
                        }
                        if (containsMouse) {
                            cursorHideTimer.restart()
                        } else {
                            cursorHideTimer.stop()
                        }
                    }

                    Timer {
                        id: cursorHideTimer
                        interval: 3000
                        repeat: false
                        onTriggered: {
                            if (imageMouseArea.containsMouse && !imageMouseArea.isDragging) {
                                if (!imageMouseArea.cursorHidden) {
                                    imageMouseArea.cursorHidden = true
                                    if (App && App["setCursorHidden"]) App["setCursorHidden"](hdrWindow, true)
                                }
                            }
                        }
                    }
                    
                    property bool isDragging: false
                    property real dragStartX: 0
                    property real dragStartY: 0
                    property real contentStartX: 0
                    property real contentStartY: 0

                    onEntered: {
                        showCursorAndArmHideTimer()
                    }

                    onExited: {
                        cursorHideTimer.stop()
                        if (cursorHidden) {
                            cursorHidden = false
                            if (App && App["setCursorHidden"]) App["setCursorHidden"](hdrWindow, false)
                        }
                    }

                    onContainsMouseChanged: {
                        if (containsMouse) {
                            showCursorAndArmHideTimer()
                        } else {
                            cursorHideTimer.stop()
                            if (cursorHidden) {
                                cursorHidden = false
                                if (App && App["setCursorHidden"]) App["setCursorHidden"](hdrWindow, false)
                            }
                        }
                    }
                    
                    onPressed: (mouse) => {
                        showCursorAndArmHideTimer()
                        if (mouse.button === Qt.LeftButton) {
                            if (root.zoomFactor > 1.0) {
                                isDragging = true
                                dragStartX = mouse.x
                                dragStartY = mouse.y
                                contentStartX = imageFlickable.contentX
                                contentStartY = imageFlickable.contentY
                            } else {
                                root.startWindowMove()
                            }
                        }
                    }
                    
                    onPositionChanged: (mouse) => {
                        showCursorAndArmHideTimer()
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
                        showCursorAndArmHideTimer()
                    }
                    
                    onClicked: {
                        root.clicked()
                    }
                    onDoubleClicked: root.doubleClicked()
                    
                    onWheel: (wheel) => {
                        showCursorAndArmHideTimer()
                        const angleDelta = wheel.angleDelta.y
                        // Normalize wheel input to step size factor
                        // Normal mice: ±120 per step → stepSizeFactor = ±1.0
                        // High-res mice: ±15 per step → stepSizeFactor = ±0.125
                        const stepSizeFactor = angleDelta / 120.0
                        //console.log("Wheel event: angleDelta =", angleDelta, "stepSizeFactor =", stepSizeFactor)
                        // Use smooth zoom with normalized step size factor
                        root.triggerZoom(stepSizeFactor, wheel.x, wheel.y)
                        wheel.accepted = true
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
        }
    }

    // Keyboard Handler Item
    Item {
        id: keyHandler
        anchors.fill: parent
        focus: true
        
        Keys.onPressed: (event) => {

            switch (event.key) {
            case Qt.Key_Q:
                if (event.modifiers & Qt.ControlModifier) {
                    Qt.quit()
                    event.accepted = true
                } else {
                    root.qPressed = true
                    event.accepted = true
                }
                break
                
            case Qt.Key_F:
            case Qt.Key_F11:
                root.toggleFullscreen()
                event.accepted = true
                break
                
            case Qt.Key_Escape:
                root.exitFullscreen()
                event.accepted = true
                break
                
            case Qt.Key_Right:
            case Qt.Key_PageDown:
            case Qt.Key_Space:
                App.navigateToNext()
                event.accepted = true
                break
                
            case Qt.Key_Left:
            case Qt.Key_PageUp:
            case Qt.Key_Backspace:
            case Qt.Key_Shift:
                App.navigateToPrevious()
                event.accepted = true
                break

            case Qt.Key_Q:
            case Qt.Key_Minus:
                root.qPressed = true
                event.accepted = true
                break
                
            case Qt.Key_E:
            case Qt.Key_Plus:
                root.ePressed = true
                event.accepted = true
                break
                
            case Qt.Key_0:
            case Qt.Key_Home:
                root.resetZoom()
                event.accepted = true
                break
                
            case Qt.Key_W:
                root.wPressed = true
                event.accepted = true
                break
                
            case Qt.Key_S:
                root.sPressed = true
                event.accepted = true
                break
                
            case Qt.Key_A:
                root.aPressed = true
                event.accepted = true
                break
                
            case Qt.Key_D:
                root.dPressed = true
                event.accepted = true
                break
                
            case Qt.Key_H:
                root.toggleHDRMode()
                event.accepted = true
                break
                
            case Qt.Key_P:
                root.smoothRendering = !root.smoothRendering
                event.accepted = true
                break
                
            default:
                event.accepted = false
            }
        }
        
        Keys.onReleased: (event) => {
            
            switch (event.key) {
            case Qt.Key_Q:
            case Qt.Key_Minus:
                root.qPressed = false
                event.accepted = true
                break
                
            case Qt.Key_E:
            case Qt.Key_Plus:
                root.ePressed = false
                event.accepted = true
                break
                
            case Qt.Key_W:
                root.wPressed = false
                event.accepted = true
                break
                
            case Qt.Key_S:
                root.sPressed = false
                event.accepted = true
                break
                
            case Qt.Key_A:
                root.aPressed = false
                event.accepted = true
                break
                
            case Qt.Key_D:
                root.dPressed = false
                event.accepted = true
                break
                
            default:
                event.accepted = false
            }
        }
    }

    Component.onCompleted: {
        // Initialize image navigation if source is provided
        if (root.source) {
            App.initializeImageList(root.source)
        }
        
        // Ensure keyboard focus
        keyHandler.forceActiveFocus()
    }
}
