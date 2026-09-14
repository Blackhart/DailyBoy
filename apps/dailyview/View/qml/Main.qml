import QtQuick
import QtQuick.Controls
import DailyBoy.DailyView

ApplicationWindow {
    id: root
    width: 960
    height: 540
    visible: true
    title: qsTr("DailyView")
    color: "#1a1a1e"

    ViewportItem {
        anchors.fill: parent
        model: sequenceModel
    }

    Label {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 12
        visible: sequenceModel.errorString.length > 0
        text: sequenceModel.errorString
        color: "#ff8a80"
        font.pixelSize: 14
    }
}
