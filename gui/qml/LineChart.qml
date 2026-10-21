import QtQuick

// Minimal multi-series line chart drawn with Canvas.
Canvas {
    id: chart
    property var series: []      // [{ data: [...], color: "#..", label: ".." }]
    property real minimum: 0
    property real maximum: 100
    property int capacity: 120
    property int revision: 0
    property string unit: ""
    onRevisionChanged: requestPaint()
    onWidthChanged: requestPaint()
    implicitHeight: 180

    onPaint: {
        const ctx = getContext("2d")
        ctx.reset()
        const w = width, h = height, left = 38, bottom = 18
        const pw = w - left, ph = h - bottom
        ctx.font = "11px sans-serif"
        ctx.lineWidth = 1
        for (let i = 0; i <= 4; ++i) {
            const y = ph - ph * i / 4 + 0.5
            ctx.strokeStyle = "#1d3640"
            ctx.beginPath(); ctx.moveTo(left, y); ctx.lineTo(w, y); ctx.stroke()
            ctx.fillStyle = "#4f6a72"
            ctx.fillText(Math.round(minimum + (maximum - minimum) * i / 4) + unit, 0, Math.max(10, y + 4))
        }
        for (const s of series) {
            const d = s.data || []
            ctx.strokeStyle = s.color
            ctx.lineWidth = 2
            ctx.beginPath()
            let started = false
            for (let i = 0; i < d.length; ++i) {
                const v = d[i]
                if (v === undefined || isNaN(v)) { started = false; continue }
                const x = left + pw * (capacity - d.length + i) / (capacity - 1)
                const y = ph - ph * Math.max(0, Math.min(1, (v - minimum) / (maximum - minimum)))
                if (!started) { ctx.moveTo(x, y); started = true } else ctx.lineTo(x, y)
            }
            ctx.stroke()
        }
    }
}
