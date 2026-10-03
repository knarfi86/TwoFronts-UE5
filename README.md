# Two Fronts UE5 — V0.1

Eigenständiger Unreal-Engine-5-Prototyp eines Echtzeitstrategiespiels. Menschen kämpfen mit rauen industriellen Platzhaltern gegen mechanische Synth-Platzhalter. Das frühere Spring/Recoil-Projekt unter `G:\Two Fronts` wurde nur als Dokumentationsreferenz gelesen und nicht verändert.

## Enthalten

- Datengetriebene `UPrimaryDataAsset`-Klassen für Fraktionen und Einheiten; die V0.1-Kataloge werden zur Laufzeit erzeugt, bis kuratierte Content-Assets geliefert werden.
- Humans und Synth, mit je Scout/Probe, Rifle Unit/Combat Drone, Battle Tank/Walker und Repair Rig/Reconstructor.
- Zwei fraktionseigene Fabriken mit vier Optionen, kostenloser FIFO-Produktionswarteschlange, Bauzeit, Spawnpunkt und Trefferpunkten.
- RTS-Kamera: WASD, Mausrad-Zoom (900–4500), Q/E drehen. Auswahl per Klick oder Rahmen, Rechtsklick für Bewegung, Angriff und Reparatur.
- Server-RPC-Grenze für alle Spielbefehle. Der Server prüft Fraktion und Zieltyp vor Produktion, Bewegung, Angriff oder Reparatur.
- Schaden, automatisches Feuer innerhalb der Sichtweite, Cooldowns, Zerstörung und fortlaufende Support-Reparatur.
- Temporäres, dynamisch erzeugtes Testfeld mit Boden, Laufzeit-Navigation, je einer Fabrik und vier Start-Einheiten pro Seite. Es ist keine finale Karte.
- Canvas-HUD: Fraktion, Einheitenstatus, Fabrik, vier Produktionsflächen und Warteschlange. `[1]`/`[2]` wechselt Humans/Synth.

## Voraussetzungen (Stand 04.10.2026)

Die lokale Prüfung fand **keine Unreal-Engine-Installation**, keinen `UnrealEditor.exe`-Pfad, keinen MSVC-Compiler und kein MSBuild. Deshalb konnte kein Projekt generiert, kompiliert oder im Editor ausgeführt werden. Git ist verfügbar. Die zu verwendende UE5-Minor-Version bleibt bewusst offen; nach Installation muss sie zur lokalen C++-Toolchain passen.

Benötigt werden:

- Unreal Engine 5.x inklusive Editor und Navigation System
- Visual Studio 2022 Build Tools oder Visual Studio 2022 mit `Desktop development with C++`, MSVC v143 und passendem Windows SDK
- Git

## Projektstart

Nach der Installation von UE5 und VS Build Tools die `EngineAssociation` in `TwoFronts.uproject` einmalig auf die installierte UE-Version setzen (oder das Projekt über den Editor zuordnen). Dann:

```powershell
Set-Location 'G:\Two Fronts UE5'
$ue = 'C:\Program Files\Epic Games\UE_5.X'
& "$ue\Engine\Build\BatchFiles\Build.bat" TwoFrontsEditor Win64 Development "$(Resolve-Path .\TwoFronts.uproject)" -WaitMutex
& "$ue\Engine\Binaries\Win64\UnrealEditor.exe" "$(Resolve-Path .\TwoFronts.uproject)"
```

Im Editor `Play` drücken. Die Engine-Entry-Map wird von `ATFGameMode` in die temporäre Arena umgewandelt. Die eigene Fraktion ist zunächst Humans; `[2]` wählt Synth. Linksklick wählt, Ziehen bildet einen Auswahlrahmen und Rechtsklick bewegt/greift/repariert. Eigene Fabrik anklicken und einen der vier unteren Produktionsbuttons anklicken.

## Prüfung

```powershell
Set-Location 'G:\Two Fronts UE5'
powershell -ExecutionPolicy Bypass -File .\Tests\Invoke-TwoFrontsChecks.ps1
# Erst mit installierter Engine:
powershell -ExecutionPolicy Bypass -File .\Tests\Invoke-TwoFrontsChecks.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.X' -RunAutomation
```

Die erste Prüfung ist nur eine statische Schutzprüfung und wird nicht als Gameplay-Test ausgegeben. Der zweite Befehl baut das C++-Ziel und startet die echten Unreal-Automation-Tests.

Siehe [ARCHITECTURE.md](ARCHITECTURE.md), [FACTIONS.md](FACTIONS.md), [ASSET_PIPELINE.md](ASSET_PIPELINE.md) und [ROADMAP.md](ROADMAP.md).
