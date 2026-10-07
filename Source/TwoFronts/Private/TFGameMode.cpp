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
#include "EngineUtils.h"
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
void ATFGameMode::BeginPlay()
{
    Super::BeginPlay();
    CreateRuntimeDefinitions();

    if (!HasAuthority())
    {
        return;
    }

    FString MapName = GetWorld()->GetMapName();
    MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);

    // The runtime arena belongs only to the original sandbox map.
    // Real maps such as TF_BridgeTest_V04 keep their authored landscape, lighting and geometry.
    if (MapName.Equals(TEXT("Entry"), ESearchCase::IgnoreCase))
    {
        BuildTestArena();
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("Two Fronts: authored map '%s' detected; runtime test arena skipped."), *MapName);
        BuildAuthoredMapNavigation();
        BuildAuthoredMapForces();
    }

    if (FParse::Param(FCommandLine::Get(), TEXT("CombatDemoTest")))
    {
        StartCombatDemo();
    }
}
void ATFGameMode::Tick(float DeltaSeconds) { Super::Tick(DeltaSeconds); if (HasAuthority()) UpdateCombatDemo(); }
void ATFGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    const FRotator InitialCameraRotation(-60.f, -90.f, 0.f);
    if (NewPlayer && !NewPlayer->GetPawn())
    {
        if (APawn* CameraPawn = GetWorld()->SpawnActor<APawn>(DefaultPawnClass, FVector(0, 0, 2200), FRotator(0.f, InitialCameraRotation.Yaw, 0.f))) NewPlayer->Possess(CameraPawn);
    }
    if (NewPlayer)
    {
        NewPlayer->SetControlRotation(InitialCameraRotation);
        if (ATFRTSCameraPawn* CameraPawn = Cast<ATFRTSCameraPawn>(NewPlayer->GetPawn()))
        {
            CameraPawn->SetActorRotation(FRotator(0.f, InitialCameraRotation.Yaw, 0.f));
        }
    }
}
bool ATFGameMode::GetAuthoredLandscapeBounds(FBox& OutBounds, int32& OutLandscapeActorCount) const
{
    OutBounds = FBox(ForceInit);
    OutLandscapeActorCount = 0;

    if (!GetWorld())
    {
        return false;
    }

    // Avoid a hard dependency on the Landscape module: the map already loads these actors,
    // and their native class names are stable for Landscape and LandscapeStreamingProxy.
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor* Actor = *It;
        if (!IsValid(Actor))
        {
            continue;
        }

        const FString ClassName = Actor->GetClass()->GetName();
        const bool bIsLandscape =
            ClassName.Equals(TEXT("Landscape"), ESearchCase::IgnoreCase) ||
            ClassName.Contains(TEXT("LandscapeStreamingProxy"), ESearchCase::IgnoreCase);

        if (!bIsLandscape)
        {
            continue;
        }

        const FBox ActorBounds = Actor->GetComponentsBoundingBox(true);
        if (ActorBounds.IsValid)
        {
            OutBounds += ActorBounds;
            ++OutLandscapeActorCount;
        }
    }

    return OutBounds.IsValid && OutLandscapeActorCount > 0;
}

bool ATFGameMode::FindNavigableAuthoredMapPoint(const FVector& Candidate, FVector& OutLocation, float SearchXY) const
{
    if (!GetWorld())
    {
        return false;
    }

    if (UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld()))
    {
        FNavLocation ProjectedLocation;
        const FVector QueryExtent(
            FMath::Max(100.f, SearchXY),
            FMath::Max(100.f, SearchXY),
            FMath::Max(500.f, AuthoredMapSpawnProjectionExtentZ));

        if (Nav->ProjectPointToNavigation(Candidate, ProjectedLocation, QueryExtent))
        {
            OutLocation = ProjectedLocation.Location;
            return true;
        }
    }

    // Fallback for the first runtime frame if Recast has not finished exposing a projection yet.
    // This still places the force on visible world geometry instead of at an arbitrary fixed Z.
    FHitResult Hit;
    const FVector TraceStart(Candidate.X, Candidate.Y, Candidate.Z + AuthoredMapSpawnProjectionExtentZ);
    const FVector TraceEnd(Candidate.X, Candidate.Y, Candidate.Z - AuthoredMapSpawnProjectionExtentZ);
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TwoFrontsAuthoredSpawn), false);
    if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
    {
        OutLocation = Hit.Location;
        return true;
    }

    return false;
}

bool ATFGameMode::FindAuthoredMapBasePoint(const FBox& LandscapeBounds, bool bNorthEast, FVector& OutLocation) const
{
    const FVector Center = LandscapeBounds.GetCenter();
    const FVector Extent = LandscapeBounds.GetExtent();
    const float Sign = bNorthEast ? 1.f : -1.f;

    // Start near the requested corner, then walk progressively toward the map center until
    // Recast finds a usable point. This avoids hard-coded coordinates and works on later maps too.
    const float RequestedFraction = FMath::Clamp(AuthoredMapBaseCornerFraction, .20f, .90f);
    const float Fractions[] =
    {
        RequestedFraction,
        FMath::Max(.20f, RequestedFraction - .10f),
        FMath::Max(.20f, RequestedFraction - .20f),
        FMath::Max(.20f, RequestedFraction - .30f),
        .20f
    };

    for (const float Fraction : Fractions)
    {
        const FVector Candidate(
            Center.X + Sign * Extent.X * Fraction,
            Center.Y + Sign * Extent.Y * Fraction,
            Center.Z);

        if (FindNavigableAuthoredMapPoint(Candidate, OutLocation, AuthoredMapBaseSearchRadiusXY))
        {
            return true;
        }
    }

    return false;
}

void ATFGameMode::BuildAuthoredMapNavigation()
{
    if (!bAutoCreateNavigationForAuthoredMaps || !GetWorld())
    {
        return;
    }

    FBox LandscapeBounds(ForceInit);
    int32 LandscapeActorCount = 0;
    if (!GetAuthoredLandscapeBounds(LandscapeBounds, LandscapeActorCount))
    {
        UE_LOG(LogTemp, Warning, TEXT("Two Fronts: no loaded Landscape actors found; automatic authored-map navigation was skipped."));
        return;
    }

    const FVector BoundsCenter = LandscapeBounds.GetCenter();
    FVector BoundsExtent = LandscapeBounds.GetExtent();
    BoundsExtent.X += AuthoredMapNavigationPaddingXY;
    BoundsExtent.Y += AuthoredMapNavigationPaddingXY;
    BoundsExtent.Z += AuthoredMapNavigationPaddingZ;
    BoundsExtent.Z = FMath::Max(BoundsExtent.Z, AuthoredMapNavigationMinHalfHeight);

    ANavMeshBoundsVolume* NavBounds = GetWorld()->SpawnActor<ANavMeshBoundsVolume>(BoundsCenter, FRotator::ZeroRotator);
    if (!NavBounds)
    {
        UE_LOG(LogTemp, Error, TEXT("Two Fronts: failed to spawn automatic NavMeshBoundsVolume for authored map."));
        return;
    }

    NavBounds->Tags.AddUnique(FName(TEXT("TwoFrontsAutoNav")));

    UBoxComponent* NavExtent = NewObject<UBoxComponent>(NavBounds, TEXT("AutoLandscapeNavExtent"));
    NavBounds->AddInstanceComponent(NavExtent);
    NavExtent->SetupAttachment(NavBounds->GetRootComponent());
    NavExtent->SetBoxExtent(BoundsExtent, false);
    NavExtent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    NavExtent->SetCanEverAffectNavigation(false);
    NavExtent->RegisterComponent();

    if (UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld()))
    {
        Nav->OnNavigationBoundsUpdated(NavBounds);
        Nav->Tick(0.f);
        Nav->Build();

        const FVector FullSize = BoundsExtent * 2.f;
        UE_LOG(LogTemp, Display,
            TEXT("Two Fronts: automatic authored-map navigation created from %d Landscape actor(s). Center=(%.0f, %.0f, %.0f) Size=(%.0f, %.0f, %.0f)."),
            LandscapeActorCount,
            BoundsCenter.X, BoundsCenter.Y, BoundsCenter.Z,
            FullSize.X, FullSize.Y, FullSize.Z);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Two Fronts: no NavigationSystem available; automatic navigation could not be built."));
    }
}

void ATFGameMode::BuildAuthoredMapForces()
{
    if (!bAutoSpawnForcesOnAuthoredMaps || bAuthoredMapForcesSpawned || !GetWorld() || !Humans || !Synth)
    {
        return;
    }

    FBox LandscapeBounds(ForceInit);
    int32 LandscapeActorCount = 0;
    if (!GetAuthoredLandscapeBounds(LandscapeBounds, LandscapeActorCount))
    {
        UE_LOG(LogTemp, Warning, TEXT("Two Fronts: authored-map forces were not spawned because Landscape bounds are unavailable."));
        return;
    }

    FVector HumanBase;
    FVector SynthBase;
    if (!FindAuthoredMapBasePoint(LandscapeBounds, false, HumanBase))
    {
        UE_LOG(LogTemp, Error, TEXT("Two Fronts: could not find a navigable southwest Humans start point."));
        return;
    }
    if (!FindAuthoredMapBasePoint(LandscapeBounds, true, SynthBase))
    {
        UE_LOG(LogTemp, Error, TEXT("Two Fronts: could not find a navigable northeast Synth start point."));
        return;
    }

    bAuthoredMapForcesSpawned = true;

    auto SpawnFaction = [this](UTFFactionDefinition* Definition, const FVector& Base, const FVector& EnemyBase, const TCHAR* FactoryLabel, const TCHAR* FactoryMeshPath)
    {
        FVector Forward = EnemyBase - Base;
        Forward.Z = 0.f;
        Forward = Forward.GetSafeNormal();
        if (Forward.IsNearlyZero())
        {
            Forward = FVector::ForwardVector;
        }
        const FVector Right(-Forward.Y, Forward.X, 0.f);
        const FRotator FacingRotation = Forward.Rotation();

        FVector FactoryLocation = Base;
        FactoryLocation.Z += 180.f;
        FActorSpawnParameters FactorySpawnParams;
        FactorySpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        if (ATFFactory* Factory = GetWorld()->SpawnActor<ATFFactory>(FactoryLocation, FacingRotation, FactorySpawnParams))
        {
            Factory->Faction = Definition->Faction;
            Factory->DisplayName = FText::FromString(FactoryLabel);
            Factory->ProductionOptions = Definition->Units;
            Factory->Visual->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, FactoryMeshPath));
            Factory->Visual->SetRelativeScale3D(Definition->Faction == ETFactionId::Humans ? FVector(7, 5, 3) : FVector(5, 5, 5));
            Factory->Tags.AddUnique(FName(TEXT("TwoFrontsAuthoredSpawn")));
        }

        int32 Spawned = 0;
        constexpr int32 UnitsPerRow = 5;
        const float ColumnCenter = (UnitsPerRow - 1) * .5f;

        for (int32 TypeIndex = 0; TypeIndex < Definition->Units.Num(); ++TypeIndex)
        {
            for (int32 UnitIndex = 0; UnitIndex < InitialUnitsPerCategory; ++UnitIndex)
            {
                const int32 RowWithinType = UnitIndex / UnitsPerRow;
                const int32 Column = UnitIndex % UnitsPerRow;
                const int32 GlobalRow = TypeIndex * 2 + RowWithinType;

                const FVector DesiredLocation =
                    Base +
                    Forward * (AuthoredMapFactoryToArmyDistance + GlobalRow * AuthoredMapArmyRowSpacing) +
                    Right * ((Column - ColumnCenter) * AuthoredMapArmyUnitSpacing);

                FVector SpawnSurface;
                if (!FindNavigableAuthoredMapPoint(DesiredLocation, SpawnSurface, AuthoredMapUnitProjectionRadiusXY))
                {
                    UE_LOG(LogTemp, Warning,
                        TEXT("Two Fronts: no local navigation point for %s unit %d/%d; unit skipped."),
                        *Definition->DisplayName.ToString(), TypeIndex, UnitIndex);
                    continue;
                }

                FVector SpawnLocation = SpawnSurface;
                SpawnLocation.Z += 120.f;
                FActorSpawnParameters UnitSpawnParams;
                UnitSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
                if (ATFUnit* Unit = GetWorld()->SpawnActor<ATFUnit>(SpawnLocation, FacingRotation, UnitSpawnParams))
                {
                    Unit->ApplyDefinition(Definition->Units[TypeIndex]);
                    Unit->Tags.AddUnique(FName(TEXT("TwoFrontsAuthoredSpawn")));
                    ++Spawned;
                }
            }
        }

        return Spawned;
    };

    // Requested map orientation: Humans begin in the southwest, Synth in the northeast.
    const int32 HumanUnits = SpawnFaction(Humans, HumanBase, SynthBase, TEXT("Human Factory"), TEXT("/Engine/BasicShapes/Cube.Cube"));
    const int32 SynthUnits = SpawnFaction(Synth, SynthBase, HumanBase, TEXT("Synth Factory"), TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

    UE_LOG(LogTemp, Display,
        TEXT("Two Fronts: authored-map forces spawned. Humans SW=(%.0f, %.0f, %.0f) Units=%d | Synth NE=(%.0f, %.0f, %.0f) Units=%d."),
        HumanBase.X, HumanBase.Y, HumanBase.Z, HumanUnits,
        SynthBase.X, SynthBase.Y, SynthBase.Z, SynthUnits);
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
    FVector CombatDemoOrigin(0.f, 0.f, 120.f);
    FBox LandscapeBounds(ForceInit);
    int32 LandscapeActorCount = 0;
    const bool bIsAuthoredMap = GetAuthoredLandscapeBounds(LandscapeBounds, LandscapeActorCount);
    if (bIsAuthoredMap)
    {
        FVector SurfaceLocation;
        if (FindNavigableAuthoredMapPoint(LandscapeBounds.GetCenter(), SurfaceLocation, AuthoredMapUnitProjectionRadiusXY))
        {
            CombatDemoOrigin = SurfaceLocation + FVector(0.f, 0.f, 120.f);
        }
    }

    auto SpawnCombatDemoUnitOnSurface = [this, bIsAuthoredMap, &CombatDemoOrigin](UTFUnitDefinition* Definition, const FVector& Offset)
    {
        FVector SpawnLocation = CombatDemoOrigin + Offset;
        if (bIsAuthoredMap)
        {
            FVector SurfaceLocation;
            if (FindNavigableAuthoredMapPoint(SpawnLocation, SurfaceLocation, 250.f))
            {
                SpawnLocation = SurfaceLocation + FVector(0.f, 0.f, 120.f);
            }
        }
        return SpawnCombatDemoUnit(Definition, SpawnLocation);
    };

    for (int32 Index = 0; Index < 10; ++Index)
    {
        const float Y = (Index % 5 - 2) * 180.f;
        const float XOffset = Index < 5 ? 0.f : 80.f;
        if (ATFUnit* Unit = SpawnCombatDemoUnitOnSurface(Humans->Units[Index < 5 ? 1 : 2], FVector(-250.f - XOffset, Y, 0.f))) CombatDemoHumans.Add(Unit);
        if (ATFUnit* Unit = SpawnCombatDemoUnitOnSurface(Synth->Units[Index < 5 ? 1 : 2], FVector(250.f + XOffset, Y, 0.f))) CombatDemoSynth.Add(Unit);
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
