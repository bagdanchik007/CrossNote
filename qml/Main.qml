import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window

    property string currentSection: "notes"

    width: 1180
    height: 760
    minimumWidth: 850
    minimumHeight: 560
    visible: true
    title: qsTr("CrossNote")
    color: "#f3f5f8"

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 62
            color: "#ffffff"
            border.color: "#e3e7ec"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                spacing: 12

                Rectangle {
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 32
                    radius: 9
                    color: "#5369e8"

                    Label {
                        anchors.centerIn: parent
                        text: "C"
                        color: "white"
                        font.pixelSize: 19
                        font.bold: true
                    }
                }

                Label {
                    text: qsTr("CrossNote")
                    color: "#20242c"
                    font.pixelSize: 20
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                Label {
                    text: qsTr("Lokaler Arbeitsbereich")
                    color: "#687181"
                    font.pixelSize: 13
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: 190
                color: "#20242c"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.topMargin: 22
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 8

                    Label {
                        Layout.leftMargin: 12
                        Layout.bottomMargin: 10
                        text: qsTr("ARBEITSBEREICH")
                        color: "#9ca4b2"
                        font.pixelSize: 11
                        font.bold: true
                    }

                    Button {
                        Layout.fillWidth: true
                        text: qsTr("Notizen")
                        checkable: true
                        checked: window.currentSection === "notes"
                        onClicked: window.currentSection = "notes"
                        Accessible.name: qsTr("Alle Notizen anzeigen")
                    }

                    Button {
                        Layout.fillWidth: true
                        text: qsTr("Favoriten")
                        checkable: true
                        checked: window.currentSection === "favorites"
                        onClicked: window.currentSection = "favorites"
                        Accessible.name: qsTr("Favoriten anzeigen")
                    }

                    Button {
                        Layout.fillWidth: true
                        text: qsTr("Kürzlich geöffnet")
                        checkable: true
                        checked: window.currentSection === "recent"
                        onClicked: window.currentSection = "recent"
                        Accessible.name: qsTr("Kürzlich geöffnete Notizen anzeigen")
                    }

                    Item { Layout.fillHeight: true }

                    Label {
                        Layout.fillWidth: true
                        Layout.leftMargin: 12
                        Layout.bottomMargin: 16
                        text: qsTr("CrossNote Vorschau")
                        color: "#9ca4b2"
                        font.pixelSize: 12
                    }
                }
            }

            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: 300
                color: "#ffffff"
                border.color: "#e3e7ec"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 12

                    Label {
                        Layout.fillWidth: true
                        text: window.currentSection === "favorites"
                              ? qsTr("Favoriten")
                              : window.currentSection === "recent"
                                ? qsTr("Kürzlich geöffnet")
                                : qsTr("Alle Notizen")
                        color: "#20242c"
                        font.pixelSize: 19
                        font.bold: true
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 8
                        color: "#f7f8fa"
                        border.color: "#e7eaf0"

                        Column {
                            anchors.centerIn: parent
                            width: parent.width - 36
                            spacing: 8

                            Label {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: qsTr("Noch keine Notizen")
                                color: "#3a414d"
                                font.pixelSize: 15
                                font.bold: true
                            }

                            Label {
                                anchors.horizontalCenter: parent.horizontalCenter
                                width: parent.width
                                text: qsTr("Deine Notizen werden hier angezeigt, sobald die Notizverwaltung eingerichtet ist.")
                                color: "#737c8a"
                                font.pixelSize: 13
                                wrapMode: Text.WordWrap
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                Column {
                    anchors.centerIn: parent
                    width: Math.min(parent.width - 48, 460)
                    spacing: 12

                    Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr("Willkommen bei CrossNote")
                        color: "#20242c"
                        font.pixelSize: 27
                        font.bold: true
                    }

                    Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: parent.width
                        text: qsTr("Wähle links einen Bereich aus. In der nächsten Entwicklungsphase kannst du hier Notizen erstellen und bearbeiten.")
                        color: "#687181"
                        font.pixelSize: 15
                        wrapMode: Text.WordWrap
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            color: "#ffffff"
            border.color: "#e3e7ec"
            border.width: 1

            Label {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 16
                text: qsTr("Bereit")
                color: "#687181"
                font.pixelSize: 12
            }
        }
    }
}
