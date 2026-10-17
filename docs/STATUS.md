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

## Noch offen

- Build und Laufzeitprüfung auf einem Rechner mit installiertem Qt 6.5+, CMake und Ninja
- Migrationen für künftige SQLite-Schemata
- Volltextindex FTS5 und Tests
- Synchronisierung, Website, Android und iOS
- Verpackung und Installer für Windows und Linux

Die Datenbank ist ein lokaler Index. Für den Geräteabgleich bleibt der Vault aus Markdown-Dateien maßgeblich.
