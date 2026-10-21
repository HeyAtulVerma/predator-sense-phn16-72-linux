import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    property string label
    property string glyph
    property bool active: false
    signal clicked()
    Layout.fillWidth: true
    height: 52
    color: active ? Theme.panelHi : (ma.containsMouse ? "#0b181d" : "transparent")
    Rectangle { width: 3; height: parent.height; color: Theme.accent; visible: root.active }
    Row {
        anchors.verticalCenter: parent.verticalCenter
        x: 22
        spacing: 14
        Text { text: root.glyph; font.pixelSize: 18; color: root.active ? Theme.accent : Theme.textDim; width: 22; horizontalAlignment: Text.AlignHCenter }
        Text { text: root.label; font.pixelSize: 14; font.weight: root.active ? Font.DemiBold : Font.Normal; color: root.active ? Theme.text : Theme.textDim; anchors.verticalCenter: parent.verticalCenter }
    }
    MouseArea { id: ma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.clicked() }
}
