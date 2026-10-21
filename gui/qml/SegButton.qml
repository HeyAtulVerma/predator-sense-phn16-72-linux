import QtQuick

// Segmented-control button
Rectangle {
    id: root
    property string text
    property bool selected: false
    property color tint: Theme.accent
    signal clicked()
    implicitWidth: Math.max(96, label.implicitWidth + 32)
    implicitHeight: 36
    radius: 4
    color: selected ? Qt.rgba(tint.r, tint.g, tint.b, 0.18) : (ma.containsMouse ? Theme.panelHi : "transparent")
    border.color: selected ? tint : Theme.line
    opacity: enabled ? 1 : 0.35
    Text { id: label; anchors.centerIn: parent; text: root.text; color: root.selected ? Theme.text : Theme.textDim; font.pixelSize: 13; font.weight: root.selected ? Font.DemiBold : Font.Normal }
    MouseArea { id: ma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.clicked() }
}
