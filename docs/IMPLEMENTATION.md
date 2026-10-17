# Implementierungsvertrag: lokaler Notizkern

## Datenhoheit

- Markdown-Dateien im gewählten Vault enthalten Titel und Notiztext.
- SQLite ist ein lokaler, wiederherstellbarer Index. Der Index wird außerhalb des Vaults im anwendungsspezifischen Datenordner pro Vault gespeichert.
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
- Entwurfsautospeicherung und Vorschau
- Externe Änderungen erkennen und vor Überschreiben schützen

## Nicht Bestandteil dieses Meilensteins

- Benutzerkonto, Cloud-API oder Online-Sync
- Gemeinsame Datenbank zwischen Geräten
- Erweiterungen und Graph-Visualisierung
