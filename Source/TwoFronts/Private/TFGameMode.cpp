#include "TFGameMode.h"
#include "TFFactionDefinition.h"
#include "TFUnitDefinition.h"
#include "TFUnit.h"
#include "TFFactory.h"
#include "TFPlayerController.h"
#include "TFPlayerState.h"
#include "TFRTSCameraPawn.h"
#include "TFHUD.h"
#include "Components/StaticMeshComponent.h"
#include "NavigationSystem.h"
#include "NavMesh/NavMeshBoundsVolume.h"

ATFGameMode::ATFGameMode()
{
    PlayerControllerClass = ATFPlayerController::StaticClass();
    PlayerStateClass = ATFPlayerState::StaticClass();
    DefaultPawnClass = ATFRTSCameraPawn::StaticClass();
    HUDClass = ATFHUD::StaticClass();
}
UTFUnitDefinition* ATFGameMode::MakeUnit(UTFFactionDefinition* FactionDefinition, FName Id, const FText& Name, ETUnitRole UnitRole, float HP, float Speed, float Damage, float Range, float Cooldown, float Repair, float BuildTime, const TCHAR* MeshPath)
{
    UTFUnitDefinition* Def = NewObject<UTFUnitDefinition>(FactionDefinition, Id);
    Def->UnitId = Id; Def->DisplayName = Name; Def->Faction = FactionDefinition->Faction; Def->Role = UnitRole;
    Def->MaxHealth = HP; Def->MoveSpeed = Speed; Def->SightRange = (UnitRole == ETUnitRole::Scout ? 1800.f : 1200.f);
    Def->Combat.Damage = Damage; Def->Combat.Range = Range; Def->Combat.Cooldown = Cooldown; Def->RepairPerSecond = Repair; Def->ProductionSeconds = BuildTime;
    Def->PlaceholderMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(MeshPath)); FactionDefinition->Units.Add(Def);
    return Def;
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
}
UTFFactionDefinition* ATFGameMode::GetFactionDefinition(ETFactionId Faction) const { return Faction == ETFactionId::Humans ? Humans : (Faction == ETFactionId::Synth ? Synth : nullptr); }
void ATFGameMode::BeginPlay() { Super::BeginPlay(); CreateRuntimeDefinitions(); if (HasAuthority()) BuildTestArena(); }
void ATFGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    if (NewPlayer && !NewPlayer->GetPawn())
    {
        if (APawn* CameraPawn = GetWorld()->SpawnActor<APawn>(DefaultPawnClass, FVector(0, 0, 2200), FRotator::ZeroRotator)) NewPlayer->Possess(CameraPawn);
    }
}
void ATFGameMode::BuildTestArena()
{
    // A deliberately temporary arena: all geometry and navigation are spawned at runtime, so the project has no final-map dependency.
    AActor* Ground = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
    UStaticMeshComponent* GroundMesh = NewObject<UStaticMeshComponent>(Ground); Ground->SetRootComponent(GroundMesh); GroundMesh->RegisterComponent();
    GroundMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"))); GroundMesh->SetWorldScale3D(FVector(140.f, 140.f, 1.f));
    ANavMeshBoundsVolume* NavBounds = GetWorld()->SpawnActor<ANavMeshBoundsVolume>(FVector(0, 0, 100), FRotator::ZeroRotator);
    NavBounds->GetRootComponent()->SetWorldScale3D(FVector(120.f, 120.f, 8.f));
    if (UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld())) Nav->Build();
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
    auto SpawnInitial = [this](UTFFactionDefinition* Definition, const FVector& Base, float Direction)
    {
        for (int32 Index = 0; Index < Definition->Units.Num(); ++Index)
        {
            ATFUnit* Unit = GetWorld()->SpawnActor<ATFUnit>(Base + FVector(Direction * (900 + Index * 180), (Index - 1.5f) * 300, 120), FRotator::ZeroRotator);
            Unit->ApplyDefinition(Definition->Units[Index]);
        }
    };
    SpawnInitial(Humans, HumanLocation, 1.f); SpawnInitial(Synth, SynthLocation, -1.f);
}
