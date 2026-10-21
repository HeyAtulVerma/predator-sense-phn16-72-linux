import QtQuick
import QtQuick.Controls.Basic

Switch {
    id: control
    padding: 0
    implicitWidth: 44
    implicitHeight: 28
    indicator: Rectangle {
        implicitWidth: 44; implicitHeight: 24
        x: 0; y: parent.height / 2 - height / 2
        radius: 12
        color: control.checked ? Theme.accent : Theme.line
        opacity: control.enabled ? 1 : 0.4
        Behavior on color { ColorAnimation { duration: 120 } }
        Rectangle {
            x: control.checked ? parent.width - width - 3 : 3; y: 3
            width: 18; height: 18; radius: 9
            color: control.checked ? "#ffffff" : Theme.textDim
            Behavior on x { NumberAnimation { duration: 120 } }
        }
    }
    contentItem: Item {}
}
