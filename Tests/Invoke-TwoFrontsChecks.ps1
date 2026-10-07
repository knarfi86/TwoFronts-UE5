[CmdletBinding()]
param(
    [string]$EngineRoot,
    [switch]$RunAutomation
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$RequiredSources = @(
    'TwoFronts.uproject',
    'Source/TwoFronts/TwoFronts.Build.cs',
    'Source/TwoFronts/Public/TFUnit.h',
    'Source/TwoFronts/Public/TFFormationPlanner.h',
    'Source/TwoFronts/Public/TFWeaponDefinition.h',
    'Source/TwoFronts/Public/TFCombatComponent.h',
    'Source/TwoFronts/Private/TFFormationPlanner.cpp',
    'Source/TwoFronts/Public/TFFactory.h',
    'Source/TwoFronts/Private/TFPlayerController.cpp',
    'Source/TwoFronts/Public/TFRTSCameraPawn.h',
    'Source/TwoFronts/Private/TFRTSCameraPawn.cpp',
    'Content/TwoFronts/Maps/TF_BridgeTest_V04.umap',
    'Source/TwoFronts/Private/Tests/TwoFrontsAutomationTests.cpp'
)

foreach ($RelativePath in $RequiredSources) {
    $Path = Join-Path $ProjectRoot $RelativePath
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "Erforderliche Quelldatei fehlt: $RelativePath" }
}

$UnitSource = Get-Content -Raw (Join-Path $ProjectRoot 'Source/TwoFronts/Private/TFGameMode.cpp')
foreach ($UnitId in 'HumanScout','HumanRifleUnit','HumanBattleTank','HumanRepairRig','SynthProbe','SynthCombatDrone','SynthWalker','SynthReconstructor') {
    if ($UnitSource -notmatch $UnitId) { throw "Einheitendefinition fehlt im Prototyp-Katalog: $UnitId" }
}
foreach ($RequiredRPC in 'ServerMoveUnits','ServerAttackUnits','ServerRepairUnits','ServerEnqueueFactory') {
    if ($UnitSource + (Get-Content -Raw (Join-Path $ProjectRoot 'Source/TwoFronts/Private/TFPlayerController.cpp')) -notmatch $RequiredRPC) { throw "Server-Befehlsnaht fehlt: $RequiredRPC" }
}
if ($UnitSource -notmatch 'InitialUnitsPerCategory') { throw 'Konfigurierbare Testarmee fehlt.' }
if ($UnitSource -notmatch 'BuildAuthoredMapForces') { throw 'Startarmeen fuer autorisierte Karten fehlen.' }
if ($UnitSource -notmatch 'SpawnCombatDemoUnitOnSurface') { throw 'Kampftest wird auf der neuen Karte nicht auf die Oberflaeche projiziert.' }
$CameraSource = Get-Content -Raw (Join-Path $ProjectRoot 'Source/TwoFronts/Private/TFRTSCameraPawn.cpp')
if ($CameraSource -notmatch 'LookYaw' -or (Get-Content -Raw (Join-Path $ProjectRoot 'Source/TwoFronts/Public/TFRTSCameraPawn.h')) -notmatch 'SetFreeLookActive') { throw 'Freie Rechtsklick-Kamera fehlt.' }
$ControllerSource = Get-Content -Raw (Join-Path $ProjectRoot 'Source/TwoFronts/Private/TFPlayerController.cpp')
if ($ControllerSource -notmatch 'FocusCameraOnFactionFactory' -or $ControllerSource -notmatch 'LastFactionShortcutTime') { throw 'Fabrikzentrierung per Doppeldruck fehlt.' }
$InputSource = Get-Content -Raw (Join-Path $ProjectRoot 'Config/DefaultInput.ini')
if ($InputSource -notmatch 'RTS_LookYaw' -or $InputSource -notmatch 'RTS_LookPitch') { throw 'Mausachsen fuer freie Kamera fehlen.' }
if ((Get-Content -Raw (Join-Path $ProjectRoot 'Source/TwoFronts/Private/Tests/TwoFrontsAutomationTests.cpp')) -notmatch 'FormationPlanner') { throw 'Formations-Automationstest fehlt.' }
Write-Host 'STATIC CHECK PASSED: Source structure and all eight gameplay definitions are present.' -ForegroundColor Green
Write-Host 'This is not a compilation or gameplay result.' -ForegroundColor Yellow

if (-not $RunAutomation) { return }
if ([string]::IsNullOrWhiteSpace($EngineRoot)) { throw '-RunAutomation requires -EngineRoot.' }
$BuildScript = Join-Path $EngineRoot 'Engine/Build/BatchFiles/Build.bat'
$EditorCmd = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
if (-not (Test-Path -LiteralPath $BuildScript)) { throw "UE build script not found: $BuildScript" }
if (-not (Test-Path -LiteralPath $EditorCmd)) { throw "UnrealEditor-Cmd not found: $EditorCmd" }

Push-Location $ProjectRoot
try {
    & $BuildScript TwoFrontsEditor Win64 Development (Join-Path $ProjectRoot 'TwoFronts.uproject') -WaitMutex
    if ($LASTEXITCODE -ne 0) { throw "UE C++ build failed with exit code $LASTEXITCODE" }
    & $EditorCmd (Join-Path $ProjectRoot 'TwoFronts.uproject') -unattended -nop4 -NullRHI '-ExecCmds=Automation RunTests TwoFronts.Gameplay; Quit'
    if ($LASTEXITCODE -ne 0) { throw "UE automation command failed with exit code $LASTEXITCODE" }
    Write-Host 'ENGINE BUILD AND AUTOMATION COMMAND PASSED. Inspect Saved/Logs for test results.' -ForegroundColor Green
}
finally { Pop-Location }
