# CrossNote

CrossNote ist ein plattformübergreifendes Notizprojekt. Ziel ist eine native App für Windows, Linux, Android und iOS mit Markdown-Dateien als portablem Notizformat. Website und Online-Synchronisierung sind als spätere Projektteile vorgesehen.

## Projektstatus

**Phase 01 – Projektgrundlage.** Das Repository enthält ein minimales Qt-Quick-Fenster und CMake-Konfiguration. Notizverwaltung und Speicherung sind noch nicht implementiert.

## Voraussetzungen

- CMake 3.21 oder neuer
- Ninja
- Qt 6.5 oder neuer mit Qt Quick
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
- `.github/workflows/` – CI-Konfiguration

## Dokumentation

- [Roadmap](docs/ROADMAP.md)
- [Technische Übersicht](docs/ARCHITECTURE.md)

## Plattformhinweis

Windows und Linux können ohne Apple-Geräte entwickelt werden. Für iOS-Builds und deren Signierung wird später eine passende Apple-Umgebung benötigt. macOS selbst ist vorerst kein zugesagtes Ziel.
# CrossNote
