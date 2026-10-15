import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    /*! Context SequenceModel — must not be named sequenceModel (shadows context). */
    required property var playback

    color: "#121216"
    height: 56

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 12

        Button {
            text: root.playback.playing ? qsTr("Pause") : qsTr("Play")
            onClicked: root.playback.TogglePlay()
        }

        Label {
            text: root.playback.currentFrame
            color: "#f2f2f5"
            font.pixelSize: 14
            Layout.preferredWidth: 56
            horizontalAlignment: Text.AlignHCenter
        }

        Slider {
            id: scrubber
            Layout.fillWidth: true
            from: root.playback.frameStart
            to: root.playback.frameEnd
            stepSize: 1
            value: root.playback.currentFrame
            onMoved: root.playback.Seek(Math.round(value))
        }

        Label {
            text: root.playback.frameStart + " – " + root.playback.frameEnd
            color: "#a0a0a8"
            font.pixelSize: 12
        }
    }
}
