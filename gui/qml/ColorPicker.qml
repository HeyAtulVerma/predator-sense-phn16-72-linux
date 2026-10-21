import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// Saturation/value square + hue bar + presets + hex field.
ColumnLayout {
    id: root
    property color color: "#00aec7"
    signal picked(color c)      // emitted when the user finishes a change

    property real hue: 0.53
    property real sat: 1
    property real val: 0.78
    property bool syncing: false

    function setFromColor(c) {
        syncing = true
        if (c.hsvSaturation > 0) hue = Math.max(0, c.hsvHue)
        sat = c.hsvSaturation
        val = c.hsvValue
        syncing = false
    }
    onColorChanged: if (!syncing) setFromColor(color)
    Component.onCompleted: setFromColor(color)
    function commit() {
        syncing = true
        color = Qt.hsva(hue, sat, val, 1)
        syncing = false
        picked(color)
    }

    spacing: 10

    Rectangle {
        id: sv
        Layout.fillWidth: true
        Layout.preferredHeight: 150
        radius: 4
        color: Qt.hsva(root.hue, 1, 1, 1)
        Rectangle { anchors.fill: parent; radius: 4; gradient: Gradient { orientation: Gradient.Horizontal; GradientStop { position: 0; color: "#ffffffff" } GradientStop { position: 1; color: "#00ffffff" } } }
        Rectangle { anchors.fill: parent; radius: 4; gradient: Gradient { GradientStop { position: 0; color: "#00000000" } GradientStop { position: 1; color: "#ff000000" } } }
        Rectangle {
            x: root.sat * sv.width - width / 2
            y: (1 - root.val) * sv.height - height / 2
            width: 16; height: 16; radius: 8
            color: "transparent"; border.color: "white"; border.width: 2
        }
        MouseArea {
            anchors.fill: parent
            function upd(m) { root.sat = Math.max(0, Math.min(1, m.x / width)); root.val = 1 - Math.max(0, Math.min(1, m.y / height)) }
            onPressed: (m) => upd(m)
            onPositionChanged: (m) => upd(m)
            onReleased: root.commit()
        }
    }

    Rectangle {
        id: hueBar
        Layout.fillWidth: true
        Layout.preferredHeight: 14
        radius: 7
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: "#ff0000" }
            GradientStop { position: 0.17; color: "#ffff00" }
            GradientStop { position: 0.33; color: "#00ff00" }
            GradientStop { position: 0.5; color: "#00ffff" }
            GradientStop { position: 0.67; color: "#0000ff" }
            GradientStop { position: 0.83; color: "#ff00ff" }
            GradientStop { position: 1.0; color: "#ff0000" }
        }
        Rectangle { x: root.hue * hueBar.width - 7; y: -2; width: 14; height: 18; radius: 4; color: "transparent"; border.color: "white"; border.width: 2 }
        MouseArea {
            anchors.fill: parent
            anchors.margins: -6
            function upd(m) { root.hue = Math.max(0, Math.min(0.999, (m.x - 6) / hueBar.width)); if (root.sat === 0) root.sat = 1 }
            onPressed: (m) => upd(m)
            onPositionChanged: (m) => upd(m)
            onReleased: root.commit()
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 6
        Repeater {
            model: ["#00aec7", "#ff0000", "#ff6a00", "#ffd000", "#30ff40", "#0050ff", "#9b30ff", "#ff2fa8", "#ffffff"]
            Rectangle {
                width: 18; height: 18; radius: 9
                color: modelData
                border.color: Qt.colorEqual(root.color, modelData) ? "white" : Theme.line
                border.width: 2
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { root.setFromColor(Qt.color(modelData)); root.commit() } }
            }
        }
        Item { Layout.fillWidth: true }
        TextField {
            id: hex
            Layout.preferredWidth: 78
            text: Qt.hsva(root.hue, root.sat, root.val, 1).toString().toUpperCase()
            color: Theme.text
            font.pixelSize: 12
            background: Rectangle { color: Theme.bgDeep; border.color: Theme.line; radius: 3 }
            validator: RegularExpressionValidator { regularExpression: /#?[0-9A-Fa-f]{6}/ }
            onEditingFinished: {
                const t = text.startsWith("#") ? text : "#" + text
                root.setFromColor(Qt.color(t))
                root.commit()
            }
        }
    }
}
