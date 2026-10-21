import QtQuick
import QtQuick.Layouts

// Panel with an optional title, used by every page.
Rectangle {
    id: root
    property string title: ""
    property string subtitle: ""
    default property alias content: body.data
    property int padding: 18
    color: Theme.panel
    radius: Theme.radius
    border.color: Theme.line
    implicitHeight: col.implicitHeight + padding * 2
    implicitWidth: col.implicitWidth + padding * 2

    ColumnLayout {
        id: col
        anchors.fill: parent
        anchors.margins: root.padding
        spacing: 12
        ColumnLayout {
            visible: root.title !== ""
            spacing: 2
            Text { text: root.title; color: Theme.text; font.pixelSize: 15; font.weight: Font.DemiBold; font.letterSpacing: 0.4 }
            Text { visible: root.subtitle !== ""; text: root.subtitle; color: Theme.textDim; font.pixelSize: 12; wrapMode: Text.WordWrap; Layout.fillWidth: true }
        }
        Item {
            id: body
            Layout.fillWidth: true
            Layout.fillHeight: true
            implicitHeight: childrenRect.height
            implicitWidth: childrenRect.width
        }
    }
}
