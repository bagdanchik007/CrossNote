# CrossNote

CrossNote ist ein plattformübergreifendes Notizprojekt. Ziel ist eine native App für Windows, Linux, Android und iOS mit Markdown-Dateien als portablem Notizformat. Website und Online-Synchronisierung sind als spätere Projektteile vorgesehen.

## Projektstatus

**Lokaler Prototyp (Phase 03–04 in Arbeit).** CrossNote bietet einen Dateisystem-Vault mit Markdown-Notizen, SQLite-Metadatenindex, Suche, Favoriten, Tags, Papierkorb, Vorschau und Entwurfsautospeicherung. Sync und Webfunktionen sind noch nicht implementiert.

Aufgaben lassen sich mit Markdown-Checkboxen (`- [ ]`) in Notizen verwalten. Der Bereich „Offene Aufgaben“ sammelt Notizen mit unerledigten Punkten. Die Editor-Statuszeile zeigt Wörter, Zeichen, eine grobe Lesezeit und den Aufgabenfortschritt. Mit `Strg+Umschalt+T` fügst du eine Checkbox an der Cursorposition ein.

## Voraussetzungen

- CMake 3.21 oder neuer
- Ninja
- Qt 6.5 oder neuer mit Qt Quick, Qt Quick Dialogs und Qt SQL
- Ein C++23-fähiger Compiler
- Git

## Bauen (Windows PowerShell)

```powershell
cmake --preset debug
cmake --build --preset debug
```

Danach die erzeugte Anwendung im Build-Verzeichnis starten. Für Linux gelten dieselben CMake-Befehle, sobald Qt 6 und Ninja installiert sind.

## Ordnerstruktur

- `src/` – C++-Einstiegspunkt und spätere App-Logik
- `qml/` – Oberfläche
- `docs/` – Roadmap und technische Entscheidungen
- `src/domain/` – Notizmodell
- `src/data/` – Vault- und SQLite-Zugriff
- `src/application/` – App-Controller
- `src/presentation/` – QML-Listenmodell
- `.github/workflows/` – CI-Konfiguration

## Dokumentation

- [Projektwebsite lokal ansehen](website/README.md)
- [CrossNote-Website](website/index.html)
- [Roadmap](docs/ROADMAP.md)
- [Technische Übersicht](docs/ARCHITECTURE.md)
- [Implementierungsvertrag](docs/IMPLEMENTATION.md)
- [Projektstatus](docs/STATUS.md)

## Plattformhinweis

Windows und Linux können ohne Apple-Geräte entwickelt werden. Für iOS-Builds und deren Signierung wird später eine passende Apple-Umgebung benötigt. macOS selbst ist vorerst kein zugesagtes Ziel.
