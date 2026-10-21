pragma Singleton
import QtQuick

QtObject {
    readonly property color bg: "#071216"
    readonly property color bgDeep: "#040b0e"
    readonly property color panel: "#0d1c22"
    readonly property color panelHi: "#12262e"
    readonly property color line: "#1d3640"
    readonly property color text: "#e6f4f7"
    readonly property color textDim: "#86a3ab"
    readonly property color textFaint: "#4f6a72"
    readonly property color accent: "#00aec7"
    readonly property color accentHi: "#3fdcf0"
    readonly property color turbo: "#ff3d3d"
    readonly property color warn: "#ffb020"
    readonly property color good: "#2fd38a"

    readonly property int radius: 6
    readonly property int gap: 16
    readonly property string font: "Inter, Noto Sans, sans-serif"

    function modeColor(m) {
        switch (m) {
        case 0: return "#5aa9ff"   // Quiet
        case 1: return accent      // Balanced
        case 4: return "#ffb020"   // Performance
        case 5: return turbo       // Turbo
        case 6: return good        // Eco
        }
        return accent
    }
    function tempColor(t) {
        if (t >= 90) return turbo
        if (t >= 75) return warn
        return accent
    }
}
