import QtQuick
import QtQuick.Controls as QQC2

Item {
    id: root
    focus: true
    
    // Signals for actions
    signal toggleFullscreen()
    signal exitFullscreen()
    signal quitApplication()
    signal navigateNext()
    signal navigatePrevious()
    signal zoomIn()
    signal zoomOut()
    signal resetZoom()
    
    Keys.onPressed: (event) => {
        switch (event.key) {
        case Qt.Key_F:
        case Qt.Key_F11:
            root.toggleFullscreen()
            event.accepted = true
            break
            
        case Qt.Key_Escape:
            root.exitFullscreen()
            event.accepted = true
            break
            
        case Qt.Key_Q:
            root.quitApplication()
            event.accepted = true
            break
            
        case Qt.Key_Right:
        case Qt.Key_D:
        case Qt.Key_PageDown:
        case Qt.Key_Space:
            root.navigateNext()
            event.accepted = true
            break
            
        case Qt.Key_Left:
        case Qt.Key_A:
        case Qt.Key_PageUp:
        case Qt.Key_Backspace:
            root.navigatePrevious()
            event.accepted = true
            break
            
        case Qt.Key_Plus:
        case Qt.Key_Equal:
            root.zoomIn()
            event.accepted = true
            break
            
        case Qt.Key_Minus:
        case Qt.Key_Underscore:
            root.zoomOut()
            event.accepted = true
            break
            
        case Qt.Key_0:
        case Qt.Key_Home:
            root.resetZoom()
            event.accepted = true
            break
            
        default:
            event.accepted = false
        }
    }
}
