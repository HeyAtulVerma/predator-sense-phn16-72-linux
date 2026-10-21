import QtQuick
import QtQuick.Shapes

// 270-degree arc gauge.
Item {
    id: root
    property real value: 0
    property real maximum: 100
    property string label: ""
    property string valueText: Math.round(value)
    property string unit: ""
    property color arcColor: Theme.accent
    property bool available: true
    implicitWidth: 150
    implicitHeight: 150

    readonly property real frac: available ? Math.max(0, Math.min(1, value / maximum)) : 0
    readonly property real stroke: width * 0.07

    Shape {
        anchors.fill: parent
        layer.enabled: true
        layer.samples: 4
        ShapePath {
            strokeColor: Theme.line; strokeWidth: root.stroke; fillColor: "transparent"; capStyle: ShapePath.RoundCap
            PathAngleArc { centerX: root.width / 2; centerY: root.height / 2; radiusX: root.width / 2 - root.stroke; radiusY: radiusX; startAngle: 135; sweepAngle: 270 }
        }
        ShapePath {
            strokeColor: root.arcColor; strokeWidth: root.stroke; fillColor: "transparent"; capStyle: ShapePath.RoundCap
            PathAngleArc { centerX: root.width / 2; centerY: root.height / 2; radiusX: root.width / 2 - root.stroke; radiusY: radiusX; startAngle: 135; sweepAngle: Math.max(0.5, 270 * root.frac) }
        }
    }
    Behavior on value { NumberAnimation { duration: 400; easing.type: Easing.OutCubic } }

    Column {
        anchors.centerIn: parent
        spacing: 0
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            Text { text: root.available ? root.valueText : "—"; color: Theme.text; font.pixelSize: root.width * (root.valueText.length > 3 ? 0.16 : 0.2); font.weight: Font.DemiBold }
            Text { text: root.available ? root.unit : ""; color: Theme.textDim; font.pixelSize: root.width * 0.09; anchors.baseline: parent.children[0].baseline }
        }
        Text { anchors.horizontalCenter: parent.horizontalCenter; text: root.label; color: Theme.textDim; font.pixelSize: root.width * 0.085 }
    }
}
