import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window
    width: 1100
    height: 720
    minimumWidth: 720
    minimumHeight: 480
    visible: true
    title: qsTr("CrossNote")

    Rectangle {
        anchors.fill: parent
        color: "#f4f5f7"

        Row {
            anchors.fill: parent
            spacing: 0

            Rectangle {
                width: 260
                height: parent.height
                color: "#20242c"

                Column {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 16

                    Label {
                        text: qsTr("CrossNote")
                        color: "white"
                        font.pixelSize: 24
                        font.bold: true
                    }

                    Label {
                        text: qsTr("Deine Notizen erscheinen hier.")
                        color: "#c7cbd1"
                        wrapMode: Text.WordWrap
                        width: parent.width
                    }
                }
            }

            Item {
                width: parent.width - 260
                height: parent.height

                Column {
                    anchors.centerIn: parent
                    spacing: 12

                    Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr("Willkommen bei CrossNote")
                        font.pixelSize: 28
                        font.bold: true
                        color: "#20242c"
                    }

                    Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr("Dein Notizprojekt beginnt hier.")
                        font.pixelSize: 16
                        color: "#5c6470"
                    }
                }
            }
        }
    }
}

