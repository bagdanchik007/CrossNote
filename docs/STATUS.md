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
- QML-Startfehler werden in `%TEMP%/CrossNote-startup.log` protokolliert
- Windows-Debug-Build mit MSYS2 UCRT64 konfiguriert und erfolgreich kompiliert

## Noch offen

- Migrationen für künftige SQLite-Schemata
- Volltextindex FTS5 und Tests
- Weitere Editoroptionen, Theme-Auswahl und zusätzliche Barrierefreiheit
- Markdown-Import- und Backup-Werkzeuge
- Synchronisierung, Website, Android und iOS
- Verpackung und Installer für Windows und Linux

Die Datenbank ist ein lokaler Index. Für den Geräteabgleich bleibt der Vault aus Markdown-Dateien maßgeblich.
