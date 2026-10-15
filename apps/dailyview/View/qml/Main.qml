import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DailyBoy.DailyView

ApplicationWindow {
    id: root
    width: 960
    height: 540
    visible: true
    title: qsTr("DailyView")
    color: "#1a1a1e"

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ViewportItem {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: sequenceModel
        }

        TransportBar {
            Layout.fillWidth: true
            playback: sequenceModel
        }
    }

    Timer {
        interval: Math.round(1000 / 24)
        running: sequenceModel.playing
        repeat: true
        onTriggered: {
            var next = sequenceModel.currentFrame + 1
            if (next > sequenceModel.frameEnd) {
                next = sequenceModel.frameStart
            }
            sequenceModel.Seek(next)
        }
    }

    Label {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 68
        anchors.margins: 12
        visible: sequenceModel.errorString.length > 0
        text: sequenceModel.errorString
        color: "#ff8a80"
        font.pixelSize: 14
        z: 1
    }
}
