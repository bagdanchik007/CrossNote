import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    id: window

    property string currentSection: "notes"
    property bool previewMode: false
    property string createNoteError: ""
    property bool darkTheme: noteController.darkTheme
    readonly property color pageColor: darkTheme ? "#171a21" : "#f3f5f8"
    readonly property color panelColor: darkTheme ? "#222732" : "#ffffff"
    readonly property color lineColor: darkTheme ? "#394150" : "#e3e7ec"
    readonly property color primaryTextColor: darkTheme ? "#e7eaf0" : "#20242c"
    readonly property color bodyTextColor: darkTheme ? "#d1d6df" : "#252a33"
    readonly property color secondaryTextColor: darkTheme ? "#a0a9b8" : "#687181"
    readonly property color mutedTextColor: darkTheme ? "#a0a9b8" : "#737c8a"

    width: 1280
    height: 820
    minimumWidth: 900
    minimumHeight: 600
    visible: true
    title: qsTr("CrossNote")
    color: pageColor

    onClosing: function(close) {
        if (!window.saveEditor())
            close.accepted = false
    }

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
        saveTimer.stop()
        if (noteController.hasSelection)
            return noteController.saveCurrent(titleField.text, bodyEditor.text)
        return true
    }
    function confirmCreateNote() {
        createNoteError = ""
        if (!saveEditor())
            return
        if (noteController.createNoteWithTemplate(createTitleField.text, templatePicker.currentValue)) {
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

    FileDialog {
        id: exportDialog
        title: qsTr("Markdown-Notiz exportieren")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("Markdown-Dateien (*.md)"), qsTr("Alle Dateien (*)")]
        onAccepted: {
            if (window.saveEditor())
                noteController.exportSelectedNote(selectedFile.toLocalFile())
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
            templatePicker.currentIndex = 0
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
            ComboBox {
                id: templatePicker
                Layout.fillWidth: true
                textRole: "label"
                valueRole: "id"
                Accessible.name: qsTr("Vorlage für die neue Notiz")
                model: [
                    { id: "blank", label: qsTr("Leere Notiz") },
                    { id: "meeting", label: qsTr("Besprechungsnotiz") },
                    { id: "project", label: qsTr("Projektübersicht") },
                    { id: "daily", label: qsTr("Tagesnotiz") }
                ]
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

    Timer {
        id: searchTimer
        interval: 180
        repeat: false
        onTriggered: noteController.setSearchQuery(searchField.text)
    }

    Shortcut {
        sequence: StandardKey.New
        enabled: !createNoteDialog.visible
        onActivated: createNoteDialog.open()
    }

    Shortcut {
        sequence: StandardKey.Save
        enabled: !createNoteDialog.visible && !deleteDialog.visible
        onActivated: window.saveEditor()
    }

    Shortcut {
        sequence: StandardKey.Find
        enabled: !createNoteDialog.visible && !deleteDialog.visible
        onActivated: {
            searchField.forceActiveFocus()
            searchField.selectAll()
        }
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
            color: window.panelColor
            border.color: window.lineColor
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
                    color: window.secondaryTextColor
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
                ToolButton {
                    text: noteController.darkTheme ? "☀" : "☾"
                    Accessible.name: noteController.darkTheme ? qsTr("Helles Design aktivieren") : qsTr("Dunkles Design aktivieren")
                    ToolTip.visible: hovered
                    ToolTip.text: Accessible.name
                    onClicked: noteController.setDarkTheme(!noteController.darkTheme)
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
                        Accessible.name: qsTr("Alle aktiven Notizen anzeigen")
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
                        text: qsTr("Tagesnotiz")
                        Accessible.name: qsTr("Heutige Tagesnotiz öffnen")
                        onClicked: {
                            if (window.saveEditor() && noteController.openDailyNote())
                                searchField.clear()
                        }
                    }
                    Button {
                        Layout.fillWidth: true
                        text: qsTr("Favoriten")
                        Accessible.name: qsTr("Favoriten anzeigen")
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
                        text: qsTr("Archiv")
                        Accessible.name: qsTr("Archivierte Notizen anzeigen")
                        checkable: true
                        checked: window.currentSection === "archive"
                        onClicked: {
                            if (!saveEditor()) return
                            window.currentSection = "archive"
                            noteController.setSection("archive")
                        }
                    }
                    Button {
                        Layout.fillWidth: true
                        text: qsTr("Kürzlich geöffnet")
                        Accessible.name: qsTr("Kürzlich geöffnete Notizen anzeigen")
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
                        text: qsTr("Kürzlich bearbeitet")
                        Accessible.name: qsTr("Kürzlich bearbeitete Notizen anzeigen")
                        checkable: true
                        checked: window.currentSection === "edited"
                        onClicked: {
                            if (!saveEditor()) return
                            window.currentSection = "edited"
                            noteController.setSection("edited")
                        }
                    }
                    Button {
                        Layout.fillWidth: true
                        text: qsTr("Papierkorb")
                        Accessible.name: qsTr("Papierkorb anzeigen")
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
                color: window.panelColor
                border.color: window.lineColor
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
                                    : window.currentSection === "edited"
                                      ? qsTr("Kürzlich bearbeitet")
                                      : window.currentSection === "archive"
                                        ? qsTr("Archiv")
                                        : window.currentSection === "trash" ? qsTr("Papierkorb") : qsTr("Alle Notizen")
                            color: window.primaryTextColor
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
                        onTextChanged: searchTimer.restart()
                    }
                    ToolButton {
                        visible: searchField.text.length > 0
                        text: "×"
                        Accessible.name: qsTr("Suchtext löschen")
                        onClicked: {
                            searchField.clear()
                            searchField.forceActiveFocus()
                        }
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
                            required property var updatedAt
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
                                        color: window.bodyTextColor
                                        font.pixelSize: 14
                                        font.bold: true
                                    }
                                    Label {
                                        Layout.fillWidth: true
                                        text: preview.length ? preview : qsTr("Leere Notiz")
                                        elide: Text.ElideRight
                                        color: window.mutedTextColor
                                        font.pixelSize: 12
                                    }
                                    Label {
                                        Layout.fillWidth: true
                                        text: Qt.formatDateTime(updatedAt, "dd.MM.yyyy HH:mm")
                                        color: window.mutedTextColor
                                        font.pixelSize: 10
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
                                text: searchField.text.length
                                      ? qsTr("Keine Treffer")
                                      : window.currentSection === "trash" ? qsTr("Papierkorb ist leer")
                                      : window.currentSection === "archive" ? qsTr("Archiv ist leer")
                                      : window.currentSection === "edited" ? qsTr("Nichts in den letzten sieben Tagen bearbeitet")
                                      : qsTr("Noch keine Notizen")
                                color: "#3a414d"
                                font.pixelSize: 15
                                font.bold: true
                            }
                            Label {
                                anchors.horizontalCenter: parent.horizontalCenter
                                width: parent.width
                                text: searchField.text.length
                                      ? qsTr("Versuche einen anderen Suchbegriff.")
                                      : window.currentSection === "archive" ? qsTr("Archivierte Notizen erscheinen nur in diesem Bereich.")
                                      : window.currentSection === "edited" ? qsTr("Hier erscheinen Notizen mit Änderungen aus den letzten sieben Tagen.")
                                      : qsTr("Erstelle deine erste Notiz mit der Schaltfläche oben.")
                                color: window.mutedTextColor
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
                        maximumLength: 160
                        Accessible.name: qsTr("Titel der ausgewählten Notiz")
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
                            if (!window.saveEditor()) return
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
                        Accessible.name: window.previewMode ? qsTr("Markdown-Editor anzeigen") : qsTr("Markdown-Vorschau anzeigen")
                        enabled: noteController.hasSelection
                        onClicked: {
                            if (!saveEditor()) return
                            window.previewMode = !window.previewMode
                        }
                    }
                    Button {
                        text: qsTr("Exportieren")
                        Accessible.name: qsTr("Ausgewählte Notiz als Markdown exportieren")
                        enabled: noteController.hasSelection
                        onClicked: exportDialog.open()
                    }
                    Button {
                        text: qsTr("Löschen")
                        Accessible.name: qsTr("Ausgewählte Notiz in den Papierkorb verschieben")
                        enabled: noteController.hasSelection
                        onClicked: deleteDialog.open()
                    }
                    Button {
                        visible: noteController.hasSelection && window.currentSection !== "trash"
                        text: window.currentSection === "archive" ? qsTr("Aus Archiv holen") : qsTr("Archivieren")
                        Accessible.name: window.currentSection === "archive" ? qsTr("Notiz aus dem Archiv holen") : qsTr("Notiz archivieren")
                        onClicked: noteController.setArchived(noteController.selectedId,
                                                              window.currentSection !== "archive")
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 10
                    color: window.panelColor
                    border.color: window.lineColor

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
                            color: window.bodyTextColor
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
                            color: window.primaryTextColor
                            font.pixelSize: 25
                            font.bold: true
                        }
                        Label {
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: parent.width
                            text: qsTr("Erstelle eine Notiz oder wähle links eine vorhandene aus. Deine Markdown-Dateien bleiben in deinem Vault.")
                            color: window.secondaryTextColor
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
                        color: window.secondaryTextColor
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
                        color: window.secondaryTextColor
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
                        delegate: Button {
                            required property string modelData
                            text: modelData
                            Accessible.name: qsTr("Fehlende Notiz %1 erstellen").arg(modelData)
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Erstellt eine Notiz für dieses Linkziel")
                            onClicked: {
                                if (window.saveEditor())
                                    noteController.createNote(modelData)
                            }
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
                        text: noteController.hasSelection
                              ? qsTr("%1 Wörter · %2 Zeichen").arg(noteController.selectedWordCount).arg(noteController.selectedCharacterCount)
                              : ""
                        color: window.secondaryTextColor
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            color: window.panelColor
            border.color: window.lineColor
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
