# CrossNote – Entwicklungsroadmap

## Ziel

CrossNote ist eine Notiz-App für Windows, Linux, Android und iOS. Markdown-Dateien bilden das portable Notizformat; SQLite verwaltet lokale Metadaten und Suchindizes. Eine Website und ein eigener Online-Sync-Dienst ergänzen die Apps später. iOS wird umgesetzt, sobald Zugang zu Apples Build- und Signierungsumgebung verfügbar ist. macOS ist vorerst kein Ziel.

## Phase 00 – Anforderungen und Entscheidungen

**Ergebnis:** Kernumfang und technische Leitplanken sind dokumentiert.

- MVP-Umfang festlegen: lokale Notizen, Markdown, automatische Speicherung und Suche
- Qt 6, C++23, Qt Quick und CMake als App-Stack festhalten
- Dateiformat, Notiz-IDs, Pfadregeln und Zeitstempel definieren
- Datenverantwortung festlegen: Markdown-Dateien für Inhalte, SQLite für lokale Indizes und Metadaten
- Website und Sync-Dienst als getrennte Komponenten planen
- Offene Plattformfragen, besonders iOS-Build und Signierung, dokumentieren

## Phase 01 – Entwicklungsumgebung und Repository

**Ergebnis:** Reproduzierbares lokales Grundgerüst für Windows und Linux.

- Qt 6, CMake, Ninja, C++23-Compiler und VS Code einrichten
- Git-Repository mit Verzeichnisstruktur anlegen
- Debug- und Release-CMake-Presets bereitstellen
- Compilerwarnungen aktivieren
- Minimale Qt-Quick-Anwendung kompilieren und starten
- GitHub Actions für automatische Builds ergänzen

## Phase 02 – App-Grundgerüst und Navigation

**Ergebnis:** Hauptfenster mit Seitenleiste, Notizliste und Arbeitsbereich.

- QML-Hauptfenster und App-Einstiegspunkt einrichten
- Navigation und Statusmeldungen ergänzen
- Fenstergrößen flexibel unterstützen
- Tastaturbedienung und Fokusverhalten vorbereiten

## Phase 03 – Notizen und lokaler Speicher

**Ergebnis:** Notizen lassen sich anlegen, ändern und dauerhaft speichern.

- Notizen erstellen, öffnen, bearbeiten und löschen
- Titel, Inhalt, stabile ID sowie Erstellungs- und Änderungszeit verwalten
- Notizordner auswählen
- Inhalte als UTF-8-Markdown speichern
- Automatisches Speichern und Wiederherstellung ungespeicherter Änderungen implementieren
- Sichere Schreibvorgänge und Dateifehler behandeln

## Phase 04 – Markdown-Editor und Vorschau

**Ergebnis:** Markdown-Inhalte lassen sich bearbeiten und formatiert ansehen.

- Editor und Vorschau bereitstellen
- Überschriften, Listen, Codeblöcke, Links und Bilder unterstützen
- Wechsel zwischen Bearbeiten und Vorschau ohne Inhaltsverlust ermöglichen
- Schriftgröße und grundlegende Editoroptionen speichern

## Phase 05 – Dateisystem und Import

**Ergebnis:** CrossNote kann vorhandene Markdown-Ordner verwenden.

- Unterordner, Umbenennen und Verschieben unterstützen
- Externe Änderungen erkennen und in die App übernehmen
- Fehlende, gesperrte und nicht beschreibbare Dateien verständlich behandeln
- Vorhandene Markdown-Sammlungen importieren
- Backup und Wiederherstellung für Notizdateien ermöglichen

## Phase 06 – Datenbank und Suche

**Ergebnis:** Metadaten und Volltextsuche funktionieren lokal.

- SQLite-Schema und Migrationen einrichten
- Titel, Tags, Favoriten, Pfade und Zeitstempel verwalten
- FTS5-Volltextindex integrieren
- Index aus den Dateien erneut aufbauen können
- Suche nach Titel und Inhalt anbieten
- Filter für Tags und Favoriten sowie Sortierung ergänzen
- Kürzlich geöffnete und bearbeitete Notizen anzeigen

## Phase 07 – Interne Links und Wissensnetz

**Ergebnis:** Notizen können aufeinander verweisen.

- `[[Notizname]]`-Links erkennen und öffnen
- Backlinks und nicht vorhandene Linkziele anzeigen
- Links bei Umbenennungen aktualisieren
- Verwaiste Notizen erkennen
- Verlinkungsübersicht und anschließend Graph-Ansicht anbieten

## Phase 08 – Oberfläche und Barrierefreiheit

**Ergebnis:** Einheitliche, anpassbare Oberfläche für Maus, Tastatur und Touch.

- Dark- und Light-Theme mit gespeicherter Auswahl
- Responsive Navigation und touchfreundliche Bedienelemente
- Tastaturkürzel, Dialoge und Kontextmenüs
- Schriftgrößen und Editor-Einstellungen
- Barrierefreiheit und Deutsch/Englisch-Unterstützung

## Phase 09 – Erste Desktop-Version

**Ergebnis:** Nutzbare lokale Version für Windows und Linux.

- MVP-Funktionen abnehmen
- Release-Builds, Windows-Installer und portables Paket erstellen
- Linux-Paketierung vorbereiten
- Icon und Versionsinformationen ergänzen
- Installation, Update und Deinstallation prüfen
- Installationsanleitung und bekannte Einschränkungen veröffentlichen

## Phase 10 – Lokaler Datei-Sync

**Ergebnis:** Geräte gleichen Notizdateien über einen gemeinsamen Ordner ab.

- Sync-Ordner auswählen und lokale Datenbank pro Gerät führen
- Neue, geänderte, gelöschte und umbenannte Dateien erkennen
- Abgleich beim Start und manuell ausführen
- Temporäre oder unvollständige Dateien ignorieren
- Gleichzeitige Änderungen erkennen und Konfliktkopien erstellen
- Konfliktauflösung und Indexaktualisierung integrieren

## Phase 11 – Website und Dokumentation

**Ergebnis:** Öffentliche Projektseite mit Informationen und Hilfen.

- Projektseite, Funktionsübersicht und Plattformstatus erstellen
- Downloads, Installationsanleitungen und Benutzerhandbuch veröffentlichen
- Versionshinweise, bekannte Einschränkungen und Kontaktweg ergänzen
- Mobilfreundliches Layout und Veröffentlichungsprozess einrichten

## Phase 12 – Online-Sync-Dienst

**Ergebnis:** Nutzer können ihre Notizen über CrossNote zwischen Geräten synchronisieren.

- Anforderungen an Konten, Geräte, Speicher und Datenschutz definieren
- API und Versionsmodell festlegen
- Anmeldung und Geräteverwaltung implementieren
- Notizdateien sicher hoch- und herunterladen
- Konflikte zuverlässig erkennen und in der App auflösen
- Übertragung verschlüsseln sowie Export und Kontolöschung ermöglichen
- Backups, Protokollierung und Betriebsüberwachung einrichten

## Phase 13 – Erweiterte Funktionen

**Ergebnis:** Zusätzliche Werkzeuge für tägliche Notizarbeit.

- Vorlagen und Tagesnotizen
- Aufgaben und Checkboxen
- Anhänge und Bilder
- Markdown- und PDF-Export
- Archiv, Papierkorb und Wiederherstellung
- Wort- und Zeichenzähler

## Phase 14 – Qualität und Leistung

**Ergebnis:** Stabiler Betrieb mit großen Notizsammlungen und zuverlässiger Wiederherstellung.

- Unit-, Integrations- und Sync-Tests aufbauen
- Fehlerprotokollierung und Datenbankwiederherstellung verbessern
- Große Sammlungen, Konflikte und Abbrüche prüfen
- Leistung und Speicherverbrauch messen und optimieren
- Automatische Build- und Prüf-Pipeline vervollständigen

## Phase 15 – Android

**Ergebnis:** Installierbare und getestete Android-App.

- Android-Build und Smartphone-Layout einrichten
- Touch, Bildschirmtastatur und Android-Dateizugriff integrieren
- Hintergrundwechsel und App-Lebenszyklus behandeln
- Speicherung nach Schließen und Neustart prüfen
- Sync integrieren, APK erstellen und auf realen Geräten prüfen

## Phase 16 – iOS

**Ergebnis:** Installierbare und getestete iPhone- und iPad-App.

- Zugang zu Mac/Xcode und Qt-iOS-Toolchain organisieren
- iOS-Build, Dokumentenzugriff und Signierung konfigurieren
- Touch und App-Lebenszyklus behandeln
- Speichern und Sync nach Unterbrechungen prüfen
- Auf iPhone und iPad testen und Release-Paket erstellen

## Phase 17 – CrossNote 1.0

**Ergebnis:** Erste stabile, dokumentierte Veröffentlichung.

- Version-1.0-Umfang abnehmen
- Kernfunktionen und verfügbare Plattformen prüfen
- Release-Pakete und Benutzerhandbuch finalisieren
- Einschränkungen und Versionshinweise veröffentlichen
- Version `1.0.0` bereitstellen

## MVP – erster nutzbarer Meilenstein

- Windows- und Linux-Desktop-App
- Markdown-Notizen lokal erstellen und bearbeiten
- Automatisch speichern und Änderungen wiederherstellen
- Ordner auswählen und vorhandene Markdown-Dateien öffnen
- Titel- und Volltextsuche
- Favoriten und Tags

Online-Sync, Website-Konten, Graph-Ansicht und Mobilversionen folgen nach einem stabilen lokalen Kern.
