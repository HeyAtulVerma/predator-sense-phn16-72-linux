import QtQuick
import QtQuick.Controls.Basic

Slider {
    id: control
    property color tint: Theme.accent
    background: Rectangle {
        x: control.leftPadding; y: control.topPadding + control.availableHeight / 2 - height / 2
        width: control.availableWidth; height: 4; radius: 2
        color: Theme.line
        opacity: control.enabled ? 1 : 0.4
        Rectangle { width: control.visualPosition * parent.width; height: parent.height; radius: 2; color: control.tint }
    }
    handle: Rectangle {
        x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        width: 18; height: 18; radius: 9
        color: control.pressed ? Theme.accentHi : "#ffffff"
        border.color: control.tint; border.width: 3
        visible: control.enabled
    }
}
