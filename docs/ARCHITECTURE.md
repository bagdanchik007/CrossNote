# Technische Übersicht

## Komponenten

- **Desktop- und Mobil-App:** C++23, Qt 6, Qt Quick/QML und CMake.
- **Notizformat:** UTF-8-Markdown-Dateien in einem vom Nutzer gewählten Notizordner.
- **Lokaler Index:** SQLite mit versioniertem Schema; FTS5 dient später der Volltextsuche.
- **Website:** eigenes Webprojekt für Projektinformationen, Downloads und Dokumentation.
- **Online-Sync:** separater Dienst und API; erst nach der lokalen Dateiablage und dem lokalen Abgleich.

## Leitlinien

1. Notizinhalte bleiben als Markdown-Dateien portabel und lesbar.
2. SQLite enthält abgeleitete Suchdaten und lokale Metadaten; der Index kann aus den Dateien neu erstellt werden.
3. Plattformabhängiger Dateizugriff wird hinter klaren App-Schnittstellen gekapselt.
4. Konflikte beim Sync werden sichtbar gemacht und verlieren keine Notizänderungen.
5. Website und Sync-Dienst werden getrennt von der nativen Qt-App versioniert und betrieben.

## Aktueller Stand

Das Repository enthält nur das Qt-Quick-Grundfenster und die Build-Konfiguration. Die Komponenten für Notizverwaltung, Datenbank, Website und Sync werden in späteren Phasen ergänzt.
