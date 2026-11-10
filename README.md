# Libster – Bibliotheksverwaltungssystem

Libster ist eine objektorientierte Bibliotheksverwaltung in C++17 mit Qt-6-GUI.
Bücher, CDs und Ausleiher werden in flachen Binärdateien gespeichert; die
`.bin`-Dateien sind die einzige dauerhafte Datenquelle. Die Core-Schicht ist
vollständig Qt-frei und separat testbar.

## Funktionen

- Bücher und CDs anlegen, bearbeiten und löschen
- Suche in Büchern und CDs (Titel, Autor/Interpret, ISBN, Verlag, Standort)
- Soft Delete mit Schutz vor dem Löschen von Medien mit offenen Ausleihen
- Zentrale Validierung (Pflichtfelder, Feldlängen, E-Mail, ISBN-10/ISBN-13)
- Ausleiherverwaltung, Ausleihe/Rückgabe
- Tagesbericht mit Ausleihen und Rückgaben pro Tag

## Technik

| Bereich      | Verwendung                                      |
| ------------ | ----------------------------------------------- |
| Sprache      | C++17                                            |
| GUI          | Qt 6 Widgets, Model/View (`QTableView`)          |
| Build        | CMake ≥ 3.21                                     |
| Persistenz   | Binärdateien mit Dateiheader, gepackte Records   |
| Tests        | Eigenes leichtgewichtiges Test-Harness via CTest |

## Voraussetzungen

- CMake ≥ 3.21
- Qt 6 (Widgets)
- C++17-Compiler (AppleClang, GCC oder MSVC 2022)

## Bauen

```bash
cmake -S . -B build
cmake --build build -j 8
```

Nach dem Hinzufügen neuer `Q_OBJECT`-Klassen empfiehlt sich ein sauberer Build,
damit MOC die Dateien sicher neu erzeugt:

```bash
cmake --build build --clean-first
```

## Starten

```bash
./build/libster
```

Beim Start fragt ein Dialog nach dem **Datenverzeichnis** und der
**erfassenden Person**. Die Angaben werden über `QSettings` gespeichert und
beim nächsten Start wiederverwendet. Fehlende `.bin`-Dateien werden beim
Öffnen automatisch angelegt.

## Tests

```bash
ctest --test-dir build --output-on-failure
```

## Testdaten erzeugen

```bash
# Funktionale Testdaten
./build/libster_generate build/data-functional --force

# Performance-Daten mit 55.000 Büchern
./build/libster_generate build/data-perf \
    --books 55000 --cds 0 --borrowers 0 --loans 0 --force
```

Optionen: `--borrowers N`, `--books N`, `--cds N`, `--loans N`, `--force`.

> `--force` ersetzt vorhandene `.bin`-Dateien im Zielverzeichnis. Daten, die
> erhalten bleiben sollen, daher außerhalb von `build/` ablegen.

## Performance-Benchmark

```bash
./build/libster_benchmark build/data-perf
```

Optionen: `--runs N`, `--threshold-ms N` (Standard 150), `--min-records N`
(Standard 55000), `--log PATH` (Standard `performance.log`). Der Benchmark
sollte in einem optimierten Build (`Release`) gemessen werden.

## Projektstruktur

```text
src/core/model/         Domänenklassen (Book, CD, Borrower, LoanTransaction)
src/core/persistence/   Binärdateien, gepackte Records, Dateiheader
src/core/services/      LibraryService: CRUD, Suche, Ausleihe, Bericht
src/core/               TimeUtil, Validation, Version (Qt-frei)
src/gui/                Qt-Widgets, Dialoge, Tabellenmodelle
tests/                  Unit-Tests mit eigenem Test-Harness
tools/                  Testdaten-Generator und Such-Benchmark
```

## Datenspeicherung

Im Datenverzeichnis liegen vier Binärdateien:

```text
borrowers.bin
books.bin
cds.bin
loan_transactions.bin
```

Records sind gepackt und byte-genau festgelegt (siehe
`src/core/persistence/*Record.h`); Preise werden intern in Cent als
`uint32_t` gespeichert. Gelöschte Datensätze bleiben mit Status `DELETED`
erhalten (Soft Delete).

## Lizenzhinweis

Qt 6 wird unter der LGPLv3 mit dynamischem Linking verwendet. Es werden keine
Änderungen an Qt selbst vorgenommen.
