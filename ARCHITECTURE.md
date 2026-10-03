# Architektur

## Laufzeitfluss

```text
ATFGameMode
  ├─ erstellt UTFFactionDefinition + 8 UTFUnitDefinition zur Laufzeit
  ├─ erzeugt temporäre Arena, Navigation, 2 Fabriken, 8 Start-Einheiten
  └─ setzt RTS-Kamera, Controller, HUD und PlayerState

ATFPlayerController (lokale Eingabe)
  └─ Server RPC → Fraktionsprüfung → ATFUnit / ATFFactory
       ├─ Bewegung → AIController / Navigation
       ├─ Angriff → Ziel + Unit-Tick → ApplyDamage
       ├─ Reparatur → RepairTarget + Unit-Tick → Health.Repair
       └─ Produktion → FIFO-Queue + Factory-Tick → Spawn Unit
```

## Trennung der Verantwortlichkeiten

| Bereich | Nahtstelle | Aufgabe |
|---|---|---|
| Regeln/Konfiguration | `UTFUnitDefinition`, `UTFFactionDefinition` | Werte, Rollen, Fraktion, Platzhalter-/späteres Modell |
| Akteure | `ATFUnit`, `ATFFactory` | Replikation, Bewegung, Kampf, Produktion |
| Shared Gameplay | `UTFHealthComponent` | Trefferpunkte, Schaden, Reparatur, Tod |
| Eingabe | `ATFPlayerController`, `ATFRTSCameraPawn` | Auswahl und clientinitiierte Befehle |
| Darstellung | `Visual`, `SelectionMarker`, `ATFHUD` | austauschbare Meshes und provisorisches HUD |

`UTFUnitDefinition` ist als `UPrimaryDataAsset` implementiert. V0.1 verwendet Runtime-Instanzen, damit das leere Projekt ohne binäre `.uasset`-Dateien vollständig im Quellcode beschrieben ist. Sobald Content geliefert wird, wird pro Unit ein Data Asset unter `Content/TwoFronts/Units` angelegt und in einem Fraktions-Data-Asset registriert; die Actor-Klassen bleiben unverändert.

## Mehrspieler-Vorbereitung

Einheiten, Fabriken, Fraktionen, Produktion und Trefferpunkte replizieren. Der Controller schickt Absichten über `ServerMoveUnits`, `ServerAttackUnits`, `ServerRepairUnits` und `ServerEnqueueFactory`. Der Server prüft die gewählte Fraktion und die Zugehörigkeit. Diese Basis ersetzt keinen vollständigen Mehrspieler-Lobby-, Besitz- oder Reconnect-Fluss; V0.1 ist ein Listen-Server-Prototyp.

## Bekannte technische Grenzen

- Die Arena entsteht zur Laufzeit auf `/Engine/Maps/Entry`; sie ist kein gespeichertes `.umap`.
- Die Canvas-Oberfläche ist funktional, aber keine finale UMG-Ansicht.
- Angriffe sind direkter Schaden ohne Projektil/Deckung/Panzerung. Einheiten bleiben bodengebunden.
- Das automatische Feuer sucht bewusst nur einfache Ziele im Sichtbereich; es ist keine Strategie-KI.
