import QtQuick
import QtQuick.Layouts

// "Title / description ...... [control]" row used on the settings page.
RowLayout {
    id: root
    property string title
    property string description
    default property alias control: slot.data
    Layout.fillWidth: true
    spacing: 16
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 2
        Text { text: root.title; color: Theme.text; font.pixelSize: 14; Layout.fillWidth: true; wrapMode: Text.WordWrap }
        Text { visible: root.description !== ""; text: root.description; color: Theme.textDim; font.pixelSize: 12; Layout.fillWidth: true; wrapMode: Text.WordWrap }
    }
    Item { id: slot; implicitWidth: childrenRect.width; implicitHeight: childrenRect.height; Layout.alignment: Qt.AlignVCenter }
}
