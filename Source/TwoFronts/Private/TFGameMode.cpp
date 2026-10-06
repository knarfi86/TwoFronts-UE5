#include "TFGameMode.h"
#include "TFFactionDefinition.h"
#include "TFUnitDefinition.h"
#include "TFWeaponDefinition.h"
#include "TFUnit.h"
#include "TFHealthComponent.h"
#include "TFDamageSystem.h"
#include "TFFactory.h"
#include "TFPlayerController.h"
#include "TFPlayerState.h"
#include "TFRTSCameraPawn.h"
#include "TFHUD.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/TextureCube.h"
#include "NavigationSystem.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

ATFGameMode::ATFGameMode()
{
    PlayerControllerClass = ATFPlayerController::StaticClass();
    PlayerStateClass = ATFPlayerState::StaticClass();
    DefaultPawnClass = ATFRTSCameraPawn::StaticClass();
    HUDClass = ATFHUD::StaticClass();
    PrimaryActorTick.bCanEverTick = true;
}
UTFUnitDefinition* ATFGameMode::MakeUnit(UTFFactionDefinition* FactionDefinition, FName Id, const FText& Name, ETUnitRole UnitRole, float HP, float Speed, float Damage, float Range, float Cooldown, float Repair, float BuildTime, const TCHAR* MeshPath)
{
    UTFUnitDefinition* Def = NewObject<UTFUnitDefinition>(FactionDefinition, Id);
    Def->UnitId = Id; Def->DisplayName = Name; Def->Faction = FactionDefinition->Faction; Def->Role = UnitRole;
    Def->MaxHealth = HP; Def->MoveSpeed = Speed; Def->SightRange = (UnitRole == ETUnitRole::Scout ? 1800.f : 1200.f);
    Def->Combat.Damage = Damage; Def->Combat.Range = Range; Def->Combat.Cooldown = Cooldown; Def->AcquisitionRange = FMath::Max(Range, Def->SightRange); Def->Armor = UnitRole == ETUnitRole::Heavy ? 8.f : (UnitRole == ETUnitRole::Combat ? 2.f : 0.f); Def->TargetCategory = UnitRole == ETUnitRole::Heavy ? ETFCombatTargetCategory::Armored : ETFCombatTargetCategory::Light; Def->RepairPerSecond = Repair; Def->ProductionSeconds = BuildTime;
    Def->PlaceholderMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(MeshPath)); FactionDefinition->Units.Add(Def);
    return Def;
}
UTFWeaponDefinition* ATFGameMode::MakeWeapon(FName Id, const FText& Name, float Damage, float Range, float Reload, ETFWeaponDelivery Delivery, float ProjectileSpeed)
{
    UTFWeaponDefinition* Weapon = NewObject<UTFWeaponDefinition>(this, Id);
    Weapon->WeaponId = Id; Weapon->DisplayName = Name; Weapon->Damage = Damage; Weapon->MaximumRange = Range; Weapon->ReloadSeconds = Reload; Weapon->Delivery = Delivery; Weapon->ProjectileSpeed = ProjectileSpeed;
    Weapon->ValidTargetCategories = { ETFCombatTargetCategory::Light, ETFCombatTargetCategory::Armored };
    RuntimeWeapons.Add(Weapon);
    return Weapon;
}
void ATFGameMode::CreateRuntimeDefinitions()
{
    if (Humans && Synth) return;
    Humans = NewObject<UTFFactionDefinition>(this, TEXT("Humans"));
    Humans->Faction = ETFactionId::Humans; Humans->DisplayName = FText::FromString(TEXT("Humans")); Humans->TeamColor = FLinearColor(.75f, .33f, .08f);
    MakeUnit(Humans, TEXT("HumanScout"), FText::FromString(TEXT("Scout")), ETUnitRole::Scout, 90, 780, 8, 450, .7f, 0, 4, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    MakeUnit(Humans, TEXT("HumanRifleUnit"), FText::FromString(TEXT("Rifle Unit")), ETUnitRole::Combat, 150, 520, 16, 650, 1, 0, 6, TEXT("/Engine/BasicShapes/Cube.Cube"));
    MakeUnit(Humans, TEXT("HumanBattleTank"), FText::FromString(TEXT("Battle Tank")), ETUnitRole::Heavy, 420, 310, 42, 800, 1.6f, 0, 11, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    MakeUnit(Humans, TEXT("HumanRepairRig"), FText::FromString(TEXT("Repair Rig")), ETUnitRole::Support, 190, 420, 0, 0, 1, 28, 8, TEXT("/Engine/BasicShapes/Cone.Cone"));
    Synth = NewObject<UTFFactionDefinition>(this, TEXT("Synth"));
    Synth->Faction = ETFactionId::Synth; Synth->DisplayName = FText::FromString(TEXT("Synth")); Synth->TeamColor = FLinearColor(.05f, .65f, .82f);
    MakeUnit(Synth, TEXT("SynthProbe"), FText::FromString(TEXT("Probe")), ETUnitRole::Scout, 75, 850, 6, 500, .55f, 0, 3.5f, TEXT("/Engine/BasicShapes/Cone.Cone"));
    MakeUnit(Synth, TEXT("SynthCombatDrone"), FText::FromString(TEXT("Combat Drone")), ETUnitRole::Combat, 130, 580, 14, 700, .75f, 0, 5.5f, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    MakeUnit(Synth, TEXT("SynthWalker"), FText::FromString(TEXT("Walker")), ETUnitRole::Heavy, 360, 360, 35, 850, 1.25f, 0, 10, TEXT("/Engine/BasicShapes/Cube.Cube"));
    MakeUnit(Synth, TEXT("SynthReconstructor"), FText::FromString(TEXT("Reconstructor")), ETUnitRole::Support, 160, 450, 0, 0, 1, 35, 7, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    FTFDirectShotVFXSettings HumanLaserVFX;
    HumanLaserVFX.BoltColor = FLinearColor(1.f, .50f, .06f);
    HumanLaserVFX.CoreColor = FLinearColor(1.f, .97f, .76f);
    HumanLaserVFX.GlowColor = FLinearColor(1.f, .16f, .01f);
    HumanLaserVFX.MuzzleColor = FLinearColor(1.f, .62f, .10f);
    HumanLaserVFX.ImpactColor = FLinearColor(1.f, .29f, .02f);
    HumanLaserVFX.BoltLength = 150.f;
    HumanLaserVFX.BoltWidth = 7.f;
    HumanLaserVFX.GlowWidth = 10.f;
    HumanLaserVFX.BoltSpeed = 24000.f;
    HumanLaserVFX.BoltLifetime = .10f;
    HumanLaserVFX.TrailLength = 28.f;
    HumanLaserVFX.TrailWidth = 3.5f;
    HumanLaserVFX.BoltJitter = 2.5f;
    HumanLaserVFX.MuzzleScale = 1.25f;
    HumanLaserVFX.ImpactScale = 1.4f;
    HumanLaserVFX.LightIntensity = 22000.f;
    FTFDirectShotVFXSettings SynthLaserVFX;
    SynthLaserVFX.BoltColor = FLinearColor(.04f, 1.f, .09f);
    SynthLaserVFX.CoreColor = FLinearColor(.86f, 1.f, .88f);
    SynthLaserVFX.GlowColor = FLinearColor(.01f, .58f, .03f);
    SynthLaserVFX.MuzzleColor = FLinearColor(.12f, 1.f, .16f);
    SynthLaserVFX.ImpactColor = FLinearColor(.03f, .82f, .07f);
    SynthLaserVFX.BoltLength = 120.f;
    SynthLaserVFX.BoltWidth = 5.5f;
    SynthLaserVFX.GlowWidth = 9.f;
    SynthLaserVFX.BoltSpeed = 29000.f;
    SynthLaserVFX.BoltLifetime = .08f;
    SynthLaserVFX.TrailLength = 20.f;
    SynthLaserVFX.TrailWidth = 2.5f;
    SynthLaserVFX.MuzzleScale = 1.05f;
    SynthLaserVFX.ImpactScale = 1.3f;
    SynthLaserVFX.LightIntensity = 18000.f;
    SynthLaserVFX.VolumetricScatteringIntensity = .15f;
    UTFWeaponDefinition* HumanScoutPulse = MakeWeapon(TEXT("HumanScoutPulse"), FText::FromString(TEXT("Scout Pulse")), 8.f, 650.f, .7f, ETFWeaponDelivery::Direct);
    HumanScoutPulse->DirectShotVFX = HumanLaserVFX;
    Humans->Units[0]->Weapons.Add(HumanScoutPulse);
    UTFWeaponDefinition* HumanRifle = MakeWeapon(TEXT("HumanRifle"), FText::FromString(TEXT("Rifle")), 16.f, 900.f, 1.f, ETFWeaponDelivery::Direct);
    HumanRifle->DirectShotVFX = HumanLaserVFX;
    Humans->Units[1]->Weapons.Add(HumanRifle);
    Humans->Units[2]->Weapons.Add(MakeWeapon(TEXT("HumanTankShell"), FText::FromString(TEXT("Tank Shell")), 42.f, 1050.f, 1.6f, ETFWeaponDelivery::Projectile, 1250.f));
    UTFWeaponDefinition* SynthProbePulse = MakeWeapon(TEXT("SynthProbePulse"), FText::FromString(TEXT("Probe Pulse")), 6.f, 700.f, .55f, ETFWeaponDelivery::Direct);
    SynthProbePulse->DirectShotVFX = SynthLaserVFX;
    Synth->Units[0]->Weapons.Add(SynthProbePulse);
    UTFWeaponDefinition* SynthDroneBeam = MakeWeapon(TEXT("SynthDroneBeam"), FText::FromString(TEXT("Drone Beam")), 14.f, 950.f, .75f, ETFWeaponDelivery::Direct);
    SynthDroneBeam->DirectShotVFX = SynthLaserVFX;
    Synth->Units[1]->Weapons.Add(SynthDroneBeam);
    Synth->Units[2]->Weapons.Add(MakeWeapon(TEXT("SynthWalkerBolt"), FText::FromString(TEXT("Walker Bolt")), 35.f, 1100.f, 1.25f, ETFWeaponDelivery::Projectile, 1400.f));
}
UTFFactionDefinition* ATFGameMode::GetFactionDefinition(ETFactionId Faction) const { return Faction == ETFactionId::Humans ? Humans : (Faction == ETFactionId::Synth ? Synth : nullptr); }
void ATFGameMode::BeginPlay() { Super::BeginPlay(); CreateRuntimeDefinitions(); if (HasAuthority()) { BuildTestArena(); if (FParse::Param(FCommandLine::Get(), TEXT("CombatDemoTest"))) StartCombatDemo(); } }
void ATFGameMode::Tick(float DeltaSeconds) { Super::Tick(DeltaSeconds); if (HasAuthority()) UpdateCombatDemo(); }
void ATFGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    const FRotator InitialCameraRotation(0.f, -90.f, 0.f);
    if (ATFRTSCameraPawn* CameraPawn = Cast<ATFRTSCameraPawn>(NewPlayer ? NewPlayer->GetPawn() : nullptr))
    {
        CameraPawn->SetActorRotation(InitialCameraRotation);
    }
    if (NewPlayer && !NewPlayer->GetPawn())
    {
        if (APawn* CameraPawn = GetWorld()->SpawnActor<APawn>(DefaultPawnClass, FVector(0, 0, 2200), InitialCameraRotation)) NewPlayer->Possess(CameraPawn);
    }
}
void ATFGameMode::BuildTestArena()
{
    // A deliberately temporary arena: all geometry and navigation are spawned at runtime, so the project has no final-map dependency.
    if (ADirectionalLight* DirectionalLight = GetWorld()->SpawnActor<ADirectionalLight>(FVector::ZeroVector, FRotator(-50.f, -35.f, 0.f)))
    {
        ULightComponent* LightComponent = DirectionalLight->GetLightComponent();
        LightComponent->SetMobility(EComponentMobility::Movable);
        LightComponent->SetIntensity(10.f);
        LightComponent->SetLightColor(FLinearColor(1.f, .95f, .85f));
    }
    if (ASkyLight* SkyLight = GetWorld()->SpawnActor<ASkyLight>(FVector::ZeroVector, FRotator::ZeroRotator))
    {
        USkyLightComponent* LightComponent = SkyLight->GetLightComponent();
        LightComponent->SetMobility(EComponentMobility::Movable);
        LightComponent->SourceType = SLS_SpecifiedCubemap;
        LightComponent->SetCubemap(LoadObject<UTextureCube>(nullptr, TEXT("/Engine/MapTemplates/Sky/DaylightAmbientCubemap.DaylightAmbientCubemap")));
        LightComponent->SetIntensity(1.f);
    }
    AActor* Ground = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
    UStaticMeshComponent* GroundMesh = NewObject<UStaticMeshComponent>(Ground);
    Ground->SetRootComponent(GroundMesh);
    GroundMesh->RegisterComponent();
    GroundMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
    GroundMesh->SetWorldScale3D(FVector(140.f, 140.f, 1.f));
    GroundMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    GroundMesh->SetCollisionResponseToAllChannels(ECR_Block);
    GroundMesh->SetCanEverAffectNavigation(true);

    ANavMeshBoundsVolume* NavBounds = GetWorld()->SpawnActor<ANavMeshBoundsVolume>(FVector(0, 0, 100), FRotator::ZeroRotator);
    UBoxComponent* NavExtent = NewObject<UBoxComponent>(NavBounds, TEXT("RuntimeNavExtent"));
    NavBounds->AddInstanceComponent(NavExtent);
    NavExtent->SetupAttachment(NavBounds->GetRootComponent());
    NavExtent->SetBoxExtent(FVector(9000.f, 9000.f, 1000.f), false);
    NavExtent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    NavExtent->SetCanEverAffectNavigation(false);
    NavExtent->RegisterComponent();

    if (UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld()))
    {
        Nav->OnNavigationBoundsUpdated(NavBounds);
        Nav->Tick(0.f);
        Nav->Build();
    }
    const FVector HumanLocation(-4500, 0, 250), SynthLocation(4500, 0, 250);
    auto MakeFactory = [this](UTFFactionDefinition* Definition, const FVector& Location, const TCHAR* Label, const TCHAR* MeshPath)
    {
        ATFFactory* Factory = GetWorld()->SpawnActor<ATFFactory>(Location, FRotator::ZeroRotator);
        Factory->Faction = Definition->Faction; Factory->DisplayName = FText::FromString(Label); Factory->ProductionOptions = Definition->Units;
        Factory->Visual->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, MeshPath)); Factory->Visual->SetRelativeScale3D(Definition->Faction == ETFactionId::Humans ? FVector(7, 5, 3) : FVector(5, 5, 5));
        return Factory;
    };
    MakeFactory(Humans, HumanLocation, TEXT("Human Factory"), TEXT("/Engine/BasicShapes/Cube.Cube"));
    MakeFactory(Synth, SynthLocation, TEXT("Synth Factory"), TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    int32 SpawnedUnitCount = 0;
    auto SpawnInitial = [this, &SpawnedUnitCount](UTFFactionDefinition* Definition, const FVector& Base, float Direction)
    {
        constexpr int32 UnitsPerSpawnRow = 5;
        for (int32 TypeIndex = 0; TypeIndex < Definition->Units.Num(); ++TypeIndex)
        {
            for (int32 UnitIndex = 0; UnitIndex < InitialUnitsPerCategory; ++UnitIndex)
            {
                const int32 SpawnRow = UnitIndex / UnitsPerSpawnRow;
                const int32 SpawnColumn = UnitIndex % UnitsPerSpawnRow;
                const FVector SpawnLocation = Base + FVector(Direction * (1050.f + SpawnRow * 420.f), (TypeIndex - (Definition->Units.Num() - 1) * .5f) * 1250.f + (SpawnColumn - (UnitsPerSpawnRow - 1) * .5f) * 230.f, 120.f);
                if (ATFUnit* Unit = GetWorld()->SpawnActor<ATFUnit>(SpawnLocation, FRotator::ZeroRotator)) { Unit->ApplyDefinition(Definition->Units[TypeIndex]); ++SpawnedUnitCount; }
            }
        }
    };
    SpawnInitial(Humans, HumanLocation, 1.f); SpawnInitial(Synth, SynthLocation, -1.f);
    UE_LOG(LogTemp, Display, TEXT("Two Fronts test arena spawned %d mobile units (%d per category and faction)."), SpawnedUnitCount, InitialUnitsPerCategory);
}
ATFUnit* ATFGameMode::SpawnCombatDemoUnit(UTFUnitDefinition* Definition, const FVector& Location)
{
    if (!Definition) return nullptr;
    if (ATFUnit* Unit = GetWorld()->SpawnActor<ATFUnit>(Location, FRotator::ZeroRotator)) { Unit->ApplyDefinition(Definition); Unit->bCombatEnabled = false; return Unit; }
    return nullptr;
}
void ATFGameMode::StartCombatDemo()
{
    if (!HasAuthority()) return;
    for (ATFUnit* Unit : CombatDemoHumans) if (IsValid(Unit)) Unit->Destroy();
    for (ATFUnit* Unit : CombatDemoSynth) if (IsValid(Unit)) Unit->Destroy();
    CombatDemoHumans.Empty(); CombatDemoSynth.Empty(); bCombatDemoStarted = false; bCombatDemoFinished = false;
    for (int32 Index = 0; Index < 10; ++Index)
    {
        const float Y = (Index % 5 - 2) * 180.f;
        const float XOffset = Index < 5 ? 0.f : 80.f;
        if (ATFUnit* Unit = SpawnCombatDemoUnit(Humans->Units[Index < 5 ? 1 : 2], FVector(-250.f - XOffset, Y, 120.f))) CombatDemoHumans.Add(Unit);
        if (ATFUnit* Unit = SpawnCombatDemoUnit(Synth->Units[Index < 5 ? 1 : 2], FVector(250.f + XOffset, Y, 120.f))) CombatDemoSynth.Add(Unit);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("CombatDemoTest")) && CombatDemoHumans.Num() > 1)
    {
        FTFDamageRequest FriendlyFireProbe;
        FriendlyFireProbe.RawDamage = 50.f;
        FriendlyFireProbe.SourceActor = CombatDemoHumans[0];
        const float AppliedDamage = FTFDamageSystem::ApplyDamage(CombatDemoHumans[1], FriendlyFireProbe);
        UE_LOG(LogTemp, Display, TEXT("COMBAT_FRIENDLY_FIRE_BLOCKED Applied=%.1f HP=%.1f"), AppliedDamage, CombatDemoHumans[1]->Health->CurrentHealth);
    }
    CombatDemoStartTime = GetWorld()->GetTimeSeconds() + 3.f;
    UE_LOG(LogTemp, Display, TEXT("COMBAT_DEMO_PREPARED Humans=10 Synth=10 Countdown=3"));
}
int32 ATFGameMode::GetCombatDemoRemaining(ETFactionId Faction) const
{
    const TArray<TObjectPtr<ATFUnit>>& Units = Faction == ETFactionId::Humans ? CombatDemoHumans : CombatDemoSynth;
    int32 Remaining = 0; for (const ATFUnit* Unit : Units) if (IsValid(Unit) && Unit->Health && Unit->Health->IsAlive()) ++Remaining;
    return Remaining;
}
FString ATFGameMode::GetCombatDemoStatus() const
{
    if (CombatDemoStartTime < 0.f) return TEXT("Bereit");
    if (!bCombatDemoStarted) return FString::Printf(TEXT("Start in %.0f"), FMath::Max(0.f, CombatDemoStartTime - GetWorld()->GetTimeSeconds()));
    const int32 HumansRemaining = GetCombatDemoRemaining(ETFactionId::Humans), SynthRemaining = GetCombatDemoRemaining(ETFactionId::Synth);
    if (HumansRemaining == 0 || SynthRemaining == 0) return HumansRemaining > 0 ? TEXT("Humans siegen") : (SynthRemaining > 0 ? TEXT("Synth siegen") : TEXT("Unentschieden"));
    return TEXT("Gefecht läuft");
}
void ATFGameMode::UpdateCombatDemo()
{
    if (CombatDemoStartTime < 0.f) return;
    if (!bCombatDemoStarted && GetWorld()->GetTimeSeconds() >= CombatDemoStartTime)
    {
        bCombatDemoStarted = true;
        for (ATFUnit* Unit : CombatDemoHumans) if (IsValid(Unit)) Unit->bCombatEnabled = true;
        for (ATFUnit* Unit : CombatDemoSynth) if (IsValid(Unit)) Unit->bCombatEnabled = true;
        UE_LOG(LogTemp, Display, TEXT("COMBAT_DEMO_STARTED Humans=%d Synth=%d"), GetCombatDemoRemaining(ETFactionId::Humans), GetCombatDemoRemaining(ETFactionId::Synth));
    }
    if (bCombatDemoStarted && !bCombatDemoFinished && (GetCombatDemoRemaining(ETFactionId::Humans) == 0 || GetCombatDemoRemaining(ETFactionId::Synth) == 0))
    {
        bCombatDemoFinished = true;
        UE_LOG(LogTemp, Display, TEXT("COMBAT_DEMO_FINISHED %s"), *GetCombatDemoStatus());
    }
}
