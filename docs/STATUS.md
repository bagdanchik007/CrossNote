# Projektstatus – lokaler Prototyp

## Bereits implementiert

- Qt Quick-Oberfläche mit Navigation, Notizliste und Editor
- Vault-Auswahl und gespeicherter zuletzt verwendeter Vault
- Markdown-Dateien mit stabiler ID und Tags in YAML-Frontmatter
- SQLite-Index pro Vault im lokalen App-Datenordner
- Erstellen, Bearbeiten, Umbenennen und autospeichern
- Favoriten, Tags, Suche und zuletzt geöffnete Notizen
- Papierkorb mit Wiederherstellen
- Archiv für Notizen, die nicht mehr in den aktiven Listen erscheinen sollen
- Dateisystemüberwachung und Sicherung einer extern geänderten Fassung als Konfliktkopie
- Markdown-Vorschau, die eingebettete Bilder vor der Anzeige entfernt
- Einstellbare Editor-Schriftgröße (12–32 px), pro Gerät gespeichert und auch in der Vorschau verwendet
- Versioniertes SQLite-Schema mit WAL-Modus, Schreibwartezeit und FTS5-Volltextindex
- Suchindex wird bei Änderungen, Löschungen, Wiederherstellung und externen Dateiereignissen gepflegt
- Suchindex kann aus den Markdown-Dateien des Vaults wiederhergestellt werden
- Titel- und Inhalts-Volltextsuche sowie interaktive Tag-Filter
- Interne `[[Notizname]]`-Links mit Navigation zu vorhandenen Notizen
- Linkanalyse mit Rückverweisen, fehlenden Zielen und direktem Erstellen fehlender Notizen
- Tagesnotizen mit eindeutigem Datum und Vorlagen für Besprechungen, Projekte und Tagesnotizen
- Begrenzte Versionssicherungen bearbeiteter Markdown-Dateien im Vault
- Markdown-Export mit Schutz vor versehentlichem Überschreiben der Quelldatei
- Persistente Hell-/Dunkel-Designauswahl mit angepasster Oberfläche
- Tastenkürzel zum Erstellen, Speichern und Suchen sowie Wort- und Zeichenzähler
- Änderungen im Editor werden vor dem Schließen der Anwendung gespeichert
- Zuletzt geöffnete Notizen werden anhand ihres Öffnungszeitpunkts sortiert
- „Kürzlich bearbeitet“ zeigt Notizen mit Änderungen innerhalb der letzten sieben Tage
- Änderungsdatum jeder Notiz wird direkt in der Notizliste angezeigt
- Qt-Test/CTest deckt Notizerstellung, Import, Suche, Tags, Papierkorb, Links, Tagesnotizen, Vorlagen, Backups und Export ab
- GitHub Actions führt die vorhandene CTest-Suite auf Linux aus
- QML-Startfehler werden in `%TEMP%/CrossNote-startup.log` protokolliert
- Windows-Debug-Build mit MSYS2 UCRT64 konfiguriert und erfolgreich kompiliert

## Noch offen

- Weitere Tests für Konfliktbehandlung und Controller-Verhalten
- Synchronisierungstests, sobald ein Synchronisierungsdienst existiert
- Migrationen für künftige SQLite-Schemata
- Weitere Editoroptionen und zusätzliche Barrierefreiheit
- Markdown-Importwerkzeuge
- Synchronisierung, Website, Android und iOS
- Verpackung und Installer für Windows und Linux

Die Datenbank ist ein lokaler Index. Für den Geräteabgleich bleibt der Vault aus Markdown-Dateien maßgeblich.
