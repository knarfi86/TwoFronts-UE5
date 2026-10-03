# Asset-Pipeline für Blender → Two Fronts UE5

## Status und Verantwortlichkeiten

Das externe Team liefert finale Karten, Modelle und Audio. Codex liefert keine finalen Kunst- oder Audio-Assets, sondern Importnähte, Platzhalter und Integration. Alle jetzigen Geometrien sind Engine-Standardformen, zur Laufzeit gesetzt und ohne Produktions-Asset ersetzbar.

Die lokale Engine-Prüfung vom 04.10.2026 ergab keine installierte UE5-Version. Daher kann kein verbindlicher, versionsspezifischer Importer behauptet werden. Die verbindliche Formatauswahl erfolgt **vor dem ersten Anlieferungsexport** mit der tatsächlich installierten Engine: UE5 importiert statische und Skeletal Meshes über die offizielle FBX-Pipeline; die im Ziel-UE-Editor angezeigte unterstützte FBX-Exporter-Version ist für die Blender-Exportvorgabe maßgeblich. Bis zu dieser Bestätigung bitte keine finale Übergabe produzieren.

## Zielstruktur

```text
Content/TwoFronts/
  Core/          gemeinsame Data Assets und Materialien
  Factions/      Fraktions-Data-Assets, Farben, Symbole
  Units/Humans/  Meshes, Skelette, Animationen, Unit-Data-Assets
  Units/Synth/
  Buildings/Humans/
  Buildings/Synth/
  Maps/          ausschließlich vom Level-Team gelieferte Maps
  UI/            austauschbare Widgets, Icons und Styles
  Audio/         Sound Cues, MetaSounds/Referenzen, nicht finale Audiodaten
  Materials/     Master Materials und Instanzen
  Placeholders/  temporäre Testassets
```

## Blender-Vertrag

| Thema | Verbindliche Vorgabe |
|---|---|
| Maßstab | Blender Units = Meters, `Unit Scale = 1.0`; vor Import eine Referenzbox von 1 m prüfen. UE verwendet Zentimeter, deshalb 1 Blender-Meter = 100 UE-cm. |
| Achsen | In Blender Z oben; beim FBX-Export die UE-kompatible Umrechnung in der Exportvorschau prüfen. Positiv X ist Vorderseite der Spielfigur; eine Vorwärtsprobe gehört zur Übergabe. |
| Pivot | Einheit: Bodenmitte; Gebäude: Mittelpunkt der begehbaren Grundfläche auf Bodenniveau; bewegliche Teile/Animationen: logisch am Drehpunkt. |
| Namen | `TF_HUM_<Role>_SM`, `TF_SYN_<Role>_SM`, `TF_HUM_Factory_SM`, `TF_SYN_Factory_SM`; Skelette `SK_`, Animationen `AN_`, Materialien `M_`/`MI_`, Texturen `T_`. Keine Leerzeichen oder nachträglichen `.001`-Suffixe. |
| Format | Primär FBX für statische und Skeletal Meshes, Exporter-Version erst nach UE-Installation verbindlich festlegen. Einzeldateien, nicht ganze `.blend`-Szenen. |
| Materialien | PBR: Base Color, Normal, Roughness, Metallic, AO getrennt und nicht eingebrannt. Standardmäßig 2k, transparente/Emissive Flächen klar kennzeichnen. |
| Kollision | Separate einfache Kollisionshüllen mit UE-Namensschema `UCX_<MeshName>_01`; keine komplexe Kollisionsgeometrie als Standard. |
| Gebäude | Die gemessene Gebäudegrundfläche und eine Freifläche für `SpawnOffset` als Metadaten beilegen. Fabrik-Ausfahrt muss mindestens die größte Unit plus Sicherheitsabstand freihalten. |
| Animation | Nur bei Bedarf Skeletal Mesh mit einem Root Bone am Boden, konsistente Framerate, Idle/Move/Attack/Repair klar getrennt und als Liste beilegen. |

## Übergabe und Austausch

1. Blender-Team liefert FBX, Texturen, Abmessungen, Vorwärtsachse, Pivot-/Kollisionshinweise und Animationsliste in einem Ordner pro Asset.
2. Integration importiert nach dem Zielpfad und erstellt Materialinstanzen; Importwarnungen, LODs und Kollisionen werden im UE-Editor geprüft.
3. Für jede Rolle entsteht ein `UTFUnitDefinition`-Data Asset. `PlaceholderMesh` zeigt auf das importierte Mesh. Werte, Fähigkeiten und die C++-Klasse bleiben unverändert.
4. Das entsprechende `UTFFactionDefinition` referenziert die vier Unit-Assets. Die Fabrik-Blueprint-Unterklasse setzt nur ihr Visual und ihre vier Optionen.
5. In der Testmap bzw. finalen Map die Kollisionskontur, Spawnfläche, Boden-Navigation und Maßstab mit einer produzierten Einheit testen.
6. Erst danach können die runtimegenerierten V0.1-Kataloge durch referenzierte Content-Assets ersetzt werden. Das ist ein gezielter Integrationsschritt mit Kompilier- und Gameplay-Test, kein Dateikopieren.

## Audio und Karten

Audio wird später unter `Content/TwoFronts/Audio` als austauschbare Sound Cues bzw. passende UE-Audioreferenzen eingebunden. Diese Version erzeugt keine Stimmen, Musik oder Platzhaltersounds.

Die finale Map wird unter `Content/TwoFronts/Maps` gespeichert und als `GameDefaultMap` gesetzt. Sie muss mindestens PlayerStart, NavMeshBoundsVolume, sichtbare Team-Startzonen, zwei freigegebene Fabrik-Spawnflächen und getestete Navigation enthalten. Die dynamische V0.1-Arena ist dafür nur ein technischer Testfall.

## PowerShell-Integrationsbefehl

Nach einem in den Content Browser importierten Asset und einer Data-Asset-Zuordnung:

```powershell
Set-Location 'G:\Two Fronts UE5'
powershell -ExecutionPolicy Bypass -File .\Tests\Invoke-TwoFrontsChecks.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.X' -RunAutomation
```
