import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    id: window

    property string currentSection: "notes"
    property bool previewMode: false
    property string createNoteError: ""

    width: 1280
    height: 820
    minimumWidth: 900
    minimumHeight: 600
    visible: true
    title: qsTr("CrossNote")
    color: "#f3f5f8"

    function safeMarkdown(title, body) {
        var safeBody = body.replace(/!\[([^\]]*)\](?:\([^)]*\)|\[[^\]]*\])/g, "[$1]")
        safeBody = safeBody.replace(/<img\b[^>]*>/gi, "")
        safeBody = safeBody.replace(/<script\b[^>]*>[\s\S]*?<\/script>/gi, "")
        safeBody = safeBody.replace(/\[\[([^\]]+)\]\]/g, function(match, target) {
            var title = target.trim()
            return "[" + title + "](crossnote:" + encodeURIComponent(title) + ")"
        })
        return "# " + title + "\n\n" + safeBody
    }
    function saveEditor() {
        if (noteController.hasSelection)
            return noteController.saveCurrent(titleField.text, bodyEditor.text)
        return true
    }
    function confirmCreateNote() {
        createNoteError = ""
        if (!saveEditor())
            return
        if (noteController.createNote(createTitleField.text)) {
            searchField.clear()
            createNoteDialog.close()
        } else {
            createNoteError = noteController.errorText
        }
    }

    FolderDialog {
        id: vaultDialog
        title: qsTr("Notizordner auswählen")
        onAccepted: {
            if (!saveEditor()) {
                vaultDialog.open()
                return
            }
            noteController.setVaultPath(selectedFolder.toLocalFile())
        }
    }

    Dialog {
        id: deleteDialog
        title: qsTr("Notiz in den Papierkorb verschieben?")
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        anchors.centerIn: parent
        Label {
            text: qsTr("Die Notiz bleibt im Vault-Papierkorb und kann später wiederhergestellt werden.")
            wrapMode: Text.WordWrap
        }
        onAccepted: {
            if (!saveEditor())
                return
            noteController.deleteCurrent()
        }
    }

    Dialog {
        id: createNoteDialog
        title: qsTr("Neue Notiz erstellen")
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.NoButton
        width: Math.min(window.width - 48, 460)
        closePolicy: Popup.CloseOnEscape

        onOpened: {
            createNoteError = ""
            createTitleField.text = qsTr("Neue Notiz")
            Qt.callLater(function() {
                createTitleField.forceActiveFocus()
                createTitleField.selectAll()
            })
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 10
            Label {
                text: qsTr("Gib einen Titel für deine Markdown-Notiz ein.")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            TextField {
                id: createTitleField
                Layout.fillWidth: true
                maximumLength: 160
                placeholderText: qsTr("Notiztitel")
                Accessible.name: qsTr("Titel der neuen Notiz")
                onAccepted: window.confirmCreateNote()
            }
            Label {
                Layout.fillWidth: true
                visible: createNoteError.length > 0
                text: createNoteError
                color: "#b42318"
                wrapMode: Text.WordWrap
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                Button {
                    text: qsTr("Abbrechen")
                    onClicked: createNoteDialog.close()
                }
                Button {
                    text: qsTr("Erstellen")
                    highlighted: true
                    onClicked: window.confirmCreateNote()
                }
            }
        }
    }

    Timer {
        id: saveTimer
        interval: 650
        repeat: false
        onTriggered: saveEditor()
    }

    Shortcut {
        sequence: StandardKey.New
        enabled: !createNoteDialog.visible
        onActivated: createNoteDialog.open()
    }

    Connections {
        target: noteController
        function onCurrentNoteChanged() {
            titleField.text = noteController.selectedTitle
            bodyEditor.text = noteController.selectedContent
            tagsField.text = noteController.selectedTags.join(", ")
            window.previewMode = false
        }
        function onSelectedTagsChanged() {
            tagsField.text = noteController.selectedTags.join(", ")
        }
    }

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
                anchors.rightMargin: 18
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
                    Layout.maximumWidth: 320
                    elide: Text.ElideMiddle
                    text: noteController.vaultPath
                    color: "#687181"
                    font.pixelSize: 12
                    Accessible.name: qsTr("Aktueller Notizordner")
                }

                Button {
                    text: qsTr("Ordner wechseln")
                    onClicked: vaultDialog.open()
                }
                ToolButton {
                    text: "↻"
                    Accessible.name: qsTr("Volltextindex neu aufbauen")
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Suchindex aus den Markdown-Dateien wiederherstellen")
                    onClicked: noteController.rebuildSearchIndex()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: 184
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
                        onClicked: {
                            if (!saveEditor()) return
                            window.currentSection = "notes"
                            noteController.setSection("notes")
                        }
                    }
                    Button {
                        Layout.fillWidth: true
                        text: qsTr("Favoriten")
                        checkable: true
                        checked: window.currentSection === "favorites"
                        onClicked: {
                            if (!saveEditor()) return
                            window.currentSection = "favorites"
                            noteController.setSection("favorites")
                        }
                    }
                    Button {
                        Layout.fillWidth: true
                        text: qsTr("Kürzlich geöffnet")
                        checkable: true
                        checked: window.currentSection === "recent"
                        onClicked: {
                            if (!saveEditor()) return
                            window.currentSection = "recent"
                            noteController.setSection("recent")
                        }
                    }
                    Button {
                        Layout.fillWidth: true
                        text: qsTr("Papierkorb")
                        checkable: true
                        checked: window.currentSection === "trash"
                        onClicked: {
                            if (!saveEditor()) return
                            window.currentSection = "trash"
                            noteController.setSection("trash")
                        }
                    }

                    Item { Layout.fillHeight: true }
                    Label {
                        Layout.leftMargin: 12
                        Layout.bottomMargin: 16
                        text: qsTr("CrossNote 0.2")
                        color: "#9ca4b2"
                        font.pixelSize: 12
                    }
                }
            }

            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: 330
                color: "#ffffff"
                border.color: "#e3e7ec"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            Layout.fillWidth: true
                            text: window.currentSection === "favorites"
                                  ? qsTr("Favoriten")
                                  : window.currentSection === "recent"
                                    ? qsTr("Kürzlich geöffnet")
                                    : window.currentSection === "trash" ? qsTr("Papierkorb") : qsTr("Alle Notizen")
                            color: "#20242c"
                            font.pixelSize: 19
                            font.bold: true
                        }
                        ToolButton {
                            visible: noteController.noteModel.tagFilter.length > 0
                            text: qsTr("Tag: %1 ×").arg(noteController.noteModel.tagFilter)
                            Accessible.name: qsTr("Tagfilter entfernen")
                            onClicked: noteController.setTagFilter("")
                        }
                    }

                    TextField {
                        id: searchField
                        Layout.fillWidth: true
                        placeholderText: qsTr("Notizen durchsuchen")
                        selectByMouse: true
                        Accessible.name: qsTr("Notizen nach Titel, Text oder Tag durchsuchen")
                        onTextChanged: noteController.setSearchQuery(text)
                    }

                    Button {
                        Layout.fillWidth: true
                        text: qsTr("+  Neue Notiz") + "  (Ctrl+N)"
                        highlighted: true
                        Accessible.name: qsTr("Neue Notiz erstellen, Tastenkürzel Strg+N")
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Erstellt eine neue Markdown-Notiz")
                        onClicked: createNoteDialog.open()
                    }

                    ListView {
                        id: noteList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 4
                        model: noteController.noteModel
                        currentIndex: -1
                        Accessible.name: qsTr("Notizliste")

                        delegate: ItemDelegate {
                            required property string noteId
                            required property string title
                            required property string preview
                            required property bool favorite
                            required property var tags
                            width: noteList.width
                            highlighted: noteController.selectedId === noteId
                            onClicked: {
                                if (window.currentSection !== "trash" && noteController.selectedId !== noteId) {
                                    if (saveEditor())
                                        noteController.openNote(noteId)
                                }
                            }

                            contentItem: RowLayout {
                                spacing: 8
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 3
                                    Label {
                                        Layout.fillWidth: true
                                        text: title
                                        elide: Text.ElideRight
                                        color: "#252a33"
                                        font.pixelSize: 14
                                        font.bold: true
                                    }
                                    Label {
                                        Layout.fillWidth: true
                                        text: preview.length ? preview : qsTr("Leere Notiz")
                                        elide: Text.ElideRight
                                        color: "#737c8a"
                                        font.pixelSize: 12
                                    }
                                    Flow {
                                        Layout.fillWidth: true
                                        visible: tags.length > 0
                                        spacing: 2
                                        Repeater {
                                            model: tags
                                            delegate: ToolButton {
                                                required property string modelData
                                                text: modelData
                                                Accessible.name: qsTr("Nach Tag %1 filtern").arg(modelData)
                                                onClicked: noteController.setTagFilter(modelData)
                                            }
                                        }
                                    }
                                }
                                ToolButton {
                                    text: window.currentSection === "trash" ? "↩" : (favorite ? "★" : "☆")
                                    Accessible.name: window.currentSection === "trash" ? qsTr("Notiz wiederherstellen") : (favorite ? qsTr("Aus Favoriten entfernen") : qsTr("Zu Favoriten hinzufügen"))
                                    onClicked: window.currentSection === "trash" ? noteController.restoreFromTrash(noteId) : noteController.setFavorite(noteId, !favorite)
                                }
                            }
                        }

                        Column {
                            anchors.centerIn: parent
                            width: parent.width - 36
                            spacing: 6
                            visible: noteList.count === 0
                            Label {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: window.currentSection === "trash" ? qsTr("Papierkorb ist leer") : (searchField.text.length ? qsTr("Keine Treffer") : qsTr("Noch keine Notizen"))
                                color: "#3a414d"
                                font.pixelSize: 15
                                font.bold: true
                            }
                            Label {
                                anchors.horizontalCenter: parent.horizontalCenter
                                width: parent.width
                                text: searchField.text.length
                                      ? qsTr("Versuche einen anderen Suchbegriff.")
                                      : qsTr("Erstelle deine erste Notiz mit der Schaltfläche oben.")
                                color: "#737c8a"
                                font.pixelSize: 13
                                wrapMode: Text.WordWrap
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: 24
                spacing: 14

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    TextField {
                        id: titleField
                        Layout.fillWidth: true
                        placeholderText: qsTr("Notiztitel")
                        font.pixelSize: 25
                        font.bold: true
                        enabled: noteController.hasSelection
                        background: Rectangle { color: "transparent"; border.width: 0 }
                        onTextEdited: saveTimer.restart()
                    }
                    TextField {
                        id: tagsField
                        Layout.preferredWidth: 190
                        placeholderText: qsTr("Tags, mit Komma trennen")
                        enabled: noteController.hasSelection
                        Accessible.name: qsTr("Tags der Notiz")
                        onEditingFinished: {
                            var values = text.split(",").map(function(tag) { return tag.trim() }).filter(function(tag) { return tag.length > 0 })
                            noteController.setTags(noteController.selectedId, values)
                        }
                    }
                    ToolButton {
                        text: "A−"
                        Accessible.name: qsTr("Editor-Schrift verkleinern")
                        onClicked: noteController.setEditorFontSize(noteController.editorFontSize - 1)
                    }
                    ToolButton {
                        text: "A+"
                        Accessible.name: qsTr("Editor-Schrift vergrößern")
                        onClicked: noteController.setEditorFontSize(noteController.editorFontSize + 1)
                    }
                    Button {
                        text: window.previewMode ? qsTr("Bearbeiten") : qsTr("Vorschau")
                        enabled: noteController.hasSelection
                        onClicked: {
                            if (!saveEditor()) return
                            window.previewMode = !window.previewMode
                        }
                    }
                    Button {
                        text: qsTr("Löschen")
                        enabled: noteController.hasSelection
                        onClicked: deleteDialog.open()
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 10
                    color: "#ffffff"
                    border.color: "#e3e7ec"

                    TextArea {
                        id: bodyEditor
                        anchors.fill: parent
                        anchors.margins: 20
                        visible: !window.previewMode
                        enabled: noteController.hasSelection
                        placeholderText: qsTr("Schreibe deine Notiz in Markdown …")
                        wrapMode: TextEdit.Wrap
                        selectByMouse: true
                        persistentSelection: true
                        font.family: "monospace"
                        font.pixelSize: noteController.editorFontSize
                        background: null
                        Accessible.name: qsTr("Markdown-Editor")
                        onTextChanged: {
                            if (activeFocus)
                                saveTimer.restart()
                        }
                    }

                    Flickable {
                        anchors.fill: parent
                        anchors.margins: 22
                        clip: true
                        visible: window.previewMode && noteController.hasSelection
                        contentWidth: width
                        contentHeight: renderedMarkdown.implicitHeight
                        Text {
                            id: renderedMarkdown
                            width: parent.width
                            textFormat: Text.MarkdownText
                            text: window.safeMarkdown(titleField.text, bodyEditor.text)
                            color: "#252a33"
                            font.pixelSize: noteController.editorFontSize
                            wrapMode: Text.WordWrap
                            onLinkActivated: function(link) {
                                if (link.toString().startsWith("crossnote:"))
                                    noteController.openNoteByTitle(decodeURIComponent(link.toString().substring(10)))
                            }
                        }
                    }

                    Column {
                        anchors.centerIn: parent
                        width: Math.min(parent.width - 48, 440)
                        spacing: 10
                        visible: !noteController.hasSelection
                        Label {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: qsTr("Dein Notizbereich")
                            color: "#20242c"
                            font.pixelSize: 25
                            font.bold: true
                        }
                        Label {
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: parent.width
                            text: qsTr("Erstelle eine Notiz oder wähle links eine vorhandene aus. Deine Markdown-Dateien bleiben in deinem Vault.")
                            color: "#687181"
                            font.pixelSize: 15
                            wrapMode: Text.WordWrap
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 6
                    visible: noteController.hasSelection
                             && (noteController.selectedLinks.length > 0
                                 || noteController.selectedBacklinks.length > 0
                                 || noteController.selectedMissingLinks.length > 0)

                    Label {
                        visible: noteController.selectedLinks.length > 0
                        text: qsTr("Verlinkt:")
                        color: "#687181"
                    }
                    Repeater {
                        model: noteController.selectedLinks
                        delegate: Button {
                            required property string modelData
                            text: modelData
                            enabled: !noteController.selectedMissingLinks.includes(modelData)
                            onClicked: noteController.openNoteByTitle(modelData)
                        }
                    }
                    Label {
                        visible: noteController.selectedBacklinks.length > 0
                        text: qsTr("Rückverweise:")
                        color: "#687181"
                    }
                    Repeater {
                        model: noteController.selectedBacklinks
                        delegate: Button {
                            required property string modelData
                            text: modelData
                            onClicked: noteController.openNoteByTitle(modelData)
                        }
                    }
                    Label {
                        visible: noteController.selectedMissingLinks.length > 0
                        text: qsTr("Fehlende Ziele:")
                        color: "#b42318"
                    }
                    Repeater {
                        model: noteController.selectedMissingLinks
                        delegate: Label {
                            required property string modelData
                            text: modelData
                            color: "#b42318"
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        Layout.fillWidth: true
                        text: noteController.errorText.length ? noteController.errorText : noteController.statusText
                        color: noteController.errorText.length ? "#b42318" : "#687181"
                        elide: Text.ElideRight
                    }
                    Label {
                        text: noteController.hasSelection ? qsTr("%1 Wörter").arg(bodyEditor.text.trim().length ? bodyEditor.text.trim().split(/\\s+/).length : 0) : ""
                        color: "#687181"
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
                text: noteController.errorText.length ? qsTr("Aktion fehlgeschlagen") : noteController.statusText
                color: noteController.errorText.length ? "#b42318" : "#687181"
                font.pixelSize: 12
            }
        }
    }
}
