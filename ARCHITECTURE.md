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
       ├─ Angriff → CombatComponent + Direct/ProjectileMovement → ApplyDamage
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

## Engine-First-Entscheidungen (V0.3.1)

| Bereich | Bestehende Lösung | Native UE-5.8-Nahtstelle | Entscheidung |
|---|---|---|---|
| Bewegung und Navigation | Bewegungsbefehl im Controller | `AAIController::MoveToLocation`, `UNavigationSystemV1`, Laufzeit-NavMesh | Beibehalten; die eigene Formation berechnet nur Zielslots. |
| Direktschuss | serverseitige Zielprüfung | zentraler Schaden, visuelle Debug-Linie | Beibehalten; Gameplay-Regeln bleiben vom Effekt getrennt. |
| Kanonen-/Walker-Geschoss | eigener Actor-Tick | `UProjectileMovementComponent`, Homing und Kollisions-Overlap | Migriert; die Engine bewegt das Geschoss, der Server bewertet den Treffer. |
| Schaden und Friendly Fire | `FTFDamageSystem` + `UTFHealthComponent` | UE-Actor-/Komponenten- und Replikationsgrenzen | Beibehalten; räumliche Abfragen dürfen Regeln nur auslösen, nie umgehen. |
| UI und Eingabe | `UUserWidget`, Controller-RPCs | UMG/Slate und Unreal Input | Beibehalten; keine Spielregeln im Widget. |
| Daten | `UPrimaryDataAsset`-Klassen, Runtime-Definitionen | UE Data Assets | Beibehalten; spätere Content-Assets können dieselben Klassen konfigurieren. |

MassEntity, Niagara, Chaos, `SuggestProjectileVelocity`/`PredictProjectilePath` und Unreal Insights sind vorgemerkt, aber nicht pauschal aktiviert: Für die aktuelle 80- bis 100-Einheiten-Arena fehlt ein gemessener Engpass beziehungsweise eine Spielanforderung (Artillerie, Raketen oder physische Trümmer). Zielsuche ist der nächste Profiling-Kandidat, weil sie derzeit pro Kampftick über Einheiten iteriert.

## Bekannte technische Grenzen

- Die Arena entsteht zur Laufzeit auf `/Engine/Maps/Entry`; sie ist kein gespeichertes `.umap`.
- Die Canvas-Oberfläche ist funktional, aber keine finale UMG-Ansicht.
- Direktschüsse nutzen zentralen Direktschaden; Kanonen-/Walker-Schüsse sind sichtbare homingfähige Projektile. Es gibt noch keine Deckung oder Explosionen.
- Das automatische Feuer sucht bewusst nur einfache Ziele im Sichtbereich; es ist keine Strategie-KI.
