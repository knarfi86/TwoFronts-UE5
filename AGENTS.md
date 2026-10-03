# Arbeitsregeln — Two Fronts UE5

## Schutzbereich

- Ausschließlich in `G:\Two Fronts UE5` ändern. `G:\Two Fronts` ist nur lesbare Referenz und darf nie verändert, verschoben oder gelöscht werden.
- Änderungen klein, nachvollziehbar und in C++/Blueprint-erweiterbaren Unreal-Grenzen halten. Keine kostenpflichtigen Plugins oder externen Laufzeitabhängigkeiten hinzufügen.
- Spielentscheidungen als serverautoritative RPC-/Actor-Operation anlegen. UI und grafische Assets dürfen keine Spielregeln enthalten.
- Generierte Unreal-Ordner (`Binaries`, `Build`, `DerivedDataCache`, `Intermediate`, `Saved`) niemals committen.
- Keine Prüfung behaupten, die nicht frisch ausgeführt wurde. Statische Prüfungen sind keine Gameplay-Prüfungen.

## Asset- und Kartenregeln

- Neue Modelle nur über `UTFUnitDefinition::PlaceholderMesh` beziehungsweise entsprechende Blueprint-Unterklassen zuordnen. Keine Gameplay-Klassen auf einzelne Blender-Dateinamen fest verdrahten.
- Die finale Karte gehört dem externen Level-Team. Die dynamische V0.1-Arena weder als finale Map noch als Gestaltungsreferenz behandeln.

## PowerShell-Implementierungsbefehl

Nach jeder Änderung den passenden PowerShell-Befehl dokumentieren und, sofern möglich, ausführen:

```powershell
Set-Location 'G:\Two Fronts UE5'
powershell -ExecutionPolicy Bypass -File .\Tests\Invoke-TwoFrontsChecks.ps1
```

Für echte Engine-Prüfungen zusätzlich `-EngineRoot` und `-RunAutomation` verwenden; siehe README.
