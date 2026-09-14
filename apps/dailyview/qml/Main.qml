import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: root
    width: 960
    height: 540
    visible: true
    title: qsTr("DailyView")
    color: "#1a1a1e"

    Label {
        anchors.centerIn: parent
        text: qsTr("Hello DailyView")
        color: "#f2f2f5"
        font.pixelSize: 28
    }
}
