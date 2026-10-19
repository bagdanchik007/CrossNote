# Projektstatus – lokaler Prototyp

## Bereits implementiert

- Qt Quick-Oberfläche mit Navigation, Notizliste und Editor
- Vault-Auswahl und gespeicherter zuletzt verwendeter Vault
- Markdown-Dateien mit stabiler ID und Tags in YAML-Frontmatter
- SQLite-Index pro Vault im lokalen App-Datenordner
- Erstellen, Bearbeiten, Umbenennen und autospeichern
- Favoriten, Tags, Suche und zuletzt geöffnete Notizen
- Papierkorb mit Wiederherstellen
- Dateisystemüberwachung und Sicherung einer extern geänderten Fassung als Konfliktkopie
- Markdown-Vorschau, die eingebettete Bilder vor der Anzeige entfernt
- Einstellbare Editor-Schriftgröße (12–32 px), pro Gerät gespeichert und auch in der Vorschau verwendet
- Versioniertes SQLite-Schema mit WAL-Modus, Schreibwartezeit und FTS5-Volltextindex
- Suchindex wird bei Änderungen, Löschungen, Wiederherstellung und externen Dateiereignissen gepflegt
- Suchindex kann aus den Markdown-Dateien des Vaults wiederhergestellt werden
- Titel- und Inhalts-Volltextsuche sowie interaktive Tag-Filter
- Interne `[[Notizname]]`-Links mit Navigation zu vorhandenen Notizen
- Zuletzt geöffnete Notizen werden anhand ihres Öffnungszeitpunkts sortiert
- Qt-Test/CTest prüft Erstellen, Wiederöffnen, Volltextindex, sichere Dateinamen und plattformübergreifende Kollisionen
- QML-Startfehler werden in `%TEMP%/CrossNote-startup.log` protokolliert
- Windows-Debug-Build mit MSYS2 UCRT64 konfiguriert und erfolgreich kompiliert

## Noch offen

- Weitere Unit- und Integrationstests für externe Änderungen, Konflikte, Papierkorb und Controller
- Synchronisierungstests, sobald ein Synchronisierungsdienst existiert
- Migrationen für künftige SQLite-Schemata
- Weitere Editoroptionen, Theme-Auswahl und zusätzliche Barrierefreiheit
- Markdown-Import- und Backup-Werkzeuge
- Synchronisierung, Website, Android und iOS
- Verpackung und Installer für Windows und Linux

Die Datenbank ist ein lokaler Index. Für den Geräteabgleich bleibt der Vault aus Markdown-Dateien maßgeblich.
