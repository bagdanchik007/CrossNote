# Technische Übersicht

## Aktueller Stand

Die Qt-Quick-App bietet einen lokalen Markdown-Vault mit Notizliste, Editor, Vorschau, Autospeichern, Favoriten, Tags, Papierkorb, Konfliktkopien, internen Wiki-Links und einer pro Gerät gespeicherten Editor-Schriftgröße. SQLite speichert Metadaten und den versionierten FTS5-Suchindex; Markdown-Dateien bleiben die maßgebliche Quelle für Notizinhalte. Suchindex und Metadaten werden bei Änderungen gepflegt und der Index kann aus dem Vault neu aufgebaut werden.

## Komponenten

- **Desktop- und Mobil-App:** C++23, Qt 6, Qt Quick/QML und CMake.
- **Notizformat:** UTF-8-Markdown-Dateien in einem vom Nutzer gewählten Notizordner.
- **Lokaler Index:** SQLite mit Schema-Versionierung, WAL-Modus, Metadaten und FTS5-Volltextindex.
- **Website:** eigenes Webprojekt für Projektinformationen, Downloads und Dokumentation.
- **Online-Sync:** separater Dienst und API; erst nach der lokalen Dateiablage und dem lokalen Abgleich.

## Leitlinien

1. Notizinhalte bleiben als Markdown-Dateien portabel und lesbar.
2. SQLite enthält abgeleitete Suchdaten und lokale Metadaten; der Index kann aus den Dateien neu erstellt werden.
3. Plattformabhängiger Dateizugriff wird hinter klaren App-Schnittstellen gekapselt.
4. Konflikte beim Sync werden sichtbar gemacht und verlieren keine Notizänderungen.
5. Website und Sync-Dienst werden getrennt von der nativen Qt-App versioniert und betrieben.

## Geplante Erweiterungen

Als nächste lokale Ausbaustufen folgen automatische Tests, Markdown-Import und Backups sowie Rückverweise und eine vollständige Linkübersicht. Website, Online-Sync und mobile Apps bleiben getrennte spätere Vorhaben.
