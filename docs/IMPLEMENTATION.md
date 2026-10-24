# Implementierungsvertrag: lokaler Notizkern

## Datenhoheit

- Markdown-Dateien im gewählten Vault enthalten Titel und Notiztext.
- SQLite ist ein lokaler, wiederherstellbarer Index. Der Index wird außerhalb des Vaults im anwendungsspezifischen Datenordner pro Vault gespeichert.
- Archivstatus ist lokale Metadateninformation im SQLite-Index; archivierte Markdown-Dateien bleiben unverändert und portabel im Vault.
- Jede Datei erhält eine stabile UUID in YAML-Frontmatter. Ein erneuter Scan darf vorhandene IDs nicht verändern.
- Dateischreibvorgänge erfolgen atomar über `QSaveFile`.
- Löschen verschiebt die Datei zunächst in den `.trash`-Ordner des Vaults.

## Notizformat

```markdown
---
id: 00000000-0000-0000-0000-000000000000
tags:
  - beispiel
---

# Titel

Inhalt der Notiz.
```

Frontmatter-Felder, die CrossNote nicht versteht, bleiben beim Speichern erhalten. Dateien ohne Frontmatter werden beim Indexieren eingelesen; Metadaten ergänzt CrossNote erst beim nächsten Speichern.

## Lokale Funktionen dieses Meilensteins

- Vault auswählen und erneut öffnen
- Notizen erstellen, lesen, umbenennen, speichern und in den Papierkorb verschieben
- Titel- und Inhaltssuche, Favoriten, Tags und zuletzt geöffnete Notizen
- Offene Markdown-Checkboxen als Aufgabenbereich und Fortschrittsanzeige pro Notiz
- Wort-, Zeichen- und Lesezeitschätzung für die ausgewählte Notiz
- Entwurfsautospeicherung und Vorschau
- Externe Änderungen erkennen und vor Überschreiben schützen

## Nicht Bestandteil dieses Meilensteins

- Benutzerkonto, Cloud-API oder Online-Sync
- Gemeinsame Datenbank zwischen Geräten
- Erweiterungen und Graph-Visualisierung

## Stabilitätsverträge der lokalen APIs

- `NoteRepository` ist synchron und wird auf dem Qt-Thread verwendet, auf dem die Instanz erstellt wurde; seine SQLite-Verbindung wird nicht zwischen Threads geteilt.
- Bei Repository-Aufrufen bedeutet `false` oder `std::nullopt` einen Fehler bzw. keinen gefundenen Datensatz; optionale `QString *error`-Ausgaben enthalten nach erfolgreichem Aufruf keinen alten Fehlertext.
- Eine Notizdatei muss eine eindeutige stabile UUID haben. Ein doppelter UUID-Wert bricht den Vault-Abgleich ab und lässt die betroffenen Dateien unverändert, statt eine Notiz still zu überschreiben.
- `setTags()` speichert normalisierte Tags sowohl im SQLite-Suchindex als auch im YAML-Frontmatter und erstellt vorher eine Versionssicherung.
- Markdown-Dateien werden als UTF-8-Bytes mit identischen LF-Zeilenenden auf allen Plattformen geschrieben.
- `NoteListModel` liefert Daten ausschließlich für gültige Indizes seiner eigenen Instanz und seiner einzigen Spalte; bei gleicher Änderungszeit ist die Reihenfolge über die Notiz-ID deterministisch.
- Aufgabenmetriken zählen `- [ ]`, `* [x]` und `+ [X]` außerhalb von umschlossenen Markdown-Codeblöcken; Beispiel-Checkboxen in Codebeispielen bleiben ausgeschlossen.
- Der Wortzähler erkennt Unicode-Buchstaben und Zahlen; Frontmatter wird aus der Statistik ausgeschlossen.
- Neue SQLite-Versionen ergänzen Metadaten mit Migrationen, die nach einem unterbrochenen Schritt sicher erneut gestartet werden können.

### Datei- und Indexwiederherstellung

- Ein Vault-Wechsel wird auf einer separaten SQLite-Verbindung vorbereitet. Der aktuelle Vault bleibt nutzbar, wenn Öffnen oder Migration des nächsten Vaults fehlschlägt.
- Ein Notizspeichern schreibt Markdown atomar und aktualisiert Datenbankzeile sowie FTS5-Eintrag in einer SQL-Transaktion. Bei Index- oder Commitfehlern werden die vorherigen Markdown-Bytes wiederhergestellt.
- Beim Umbenennen wird der alte Pfad vor dem Commit entfernt. Scheitert der Datenbank-Commit, wird die vorherige Datei zurückgeschrieben und das neue Ziel entfernt.
- Pfade werden über ihren nächsten existierenden Vorfahren geprüft. Dadurch werden auch noch nicht angelegte Unterpfade abgewiesen, deren Elternsymbol-Link aus dem Vault herausführt.
- Konfliktkopien erhalten eindeutige Zeitstempel-Namen und werden erst nach erfolgreichem Speichern und Index-Commit gemeldet.
- Fehlende und unvollständige Lesevorgänge im Vault, Papierkorb, Export, Konflikt- und Wiederherstellungspfad werden als Fehler gemeldet statt unvollständigen Text zu indexieren.
