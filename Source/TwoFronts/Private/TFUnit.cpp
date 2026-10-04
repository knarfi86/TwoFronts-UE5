#include "TFUnit.h"
#include "TFHealthComponent.h"
#include "AIController.h"
#include "TFUnitDefinition.h"
#include "TFDamageSystem.h"
#include "TFCombatComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

ATFUnit::ATFUnit()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);
    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
    Visual->SetupAttachment(GetRootComponent());
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SelectionMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SelectionMarker"));
    SelectionMarker->SetupAttachment(GetRootComponent());
    SelectionMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SelectionMarker->SetVisibility(false);
    Health = CreateDefaultSubobject<UTFHealthComponent>(TEXT("Health"));
    HealthBar = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HealthBar"));
    HealthBar->SetupAttachment(GetRootComponent());
    HealthBar->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
    HealthBar->SetRelativeLocation(FVector(0.f, 0.f, 130.f));
    HealthBar->SetRelativeScale3D(FVector(1.2f, .08f, .08f));
    HealthBar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Health->OnHealthChanged.AddDynamic(this, &ATFUnit::UpdateHealthVisual);
    CombatController = CreateDefaultSubobject<UTFCombatComponent>(TEXT("CombatController"));
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.f, 720.f, 0.f);
    AIControllerClass = AAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}
void ATFUnit::ApplyDefinition(UTFUnitDefinition* InDefinition)
{
    if (!HasAuthority() || !InDefinition) return;
    Definition = InDefinition;
    Faction = InDefinition->Faction;
    Health->Initialise(InDefinition->MaxHealth);
    GetCharacterMovement()->MaxWalkSpeed = InDefinition->MoveSpeed;
    if (UStaticMesh* PlaceholderMesh = InDefinition->PlaceholderMesh.LoadSynchronous()) Visual->SetStaticMesh(PlaceholderMesh);
    if (UStaticMesh* SelectionMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")))
    {
        SelectionMarker->SetStaticMesh(SelectionMesh);
        SelectionMarker->SetRelativeLocation(FVector(0, 0, -82.f));
        SelectionMarker->SetRelativeScale3D(FVector(1.45f, 1.45f, .12f));
    }
}
void ATFUnit::SetSelectedVisual(bool bSelected) { if (SelectionMarker) SelectionMarker->SetVisibility(bSelected, true); }
float ATFUnit::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    FTFDamageRequest Request;
    Request.RawDamage = DamageAmount;
    Request.SourceActor = DamageCauser;
    const float Applied = FTFDamageSystem::ApplyDamage(this, Request);
    Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    return Applied;
}
float ATFUnit::GetArmor() const { return Definition ? Definition->Armor : 0.f; }
void ATFUnit::UpdateHealthVisual(float CurrentHealth, float MaximumHealth)
{
    if (HealthBar) HealthBar->SetRelativeScale3D(FVector(1.2f * FMath::Clamp(MaximumHealth > 0.f ? CurrentHealth / MaximumHealth : 0.f, .02f, 1.f), .08f, .08f));
}
bool ATFUnit::CanReceiveOrdersFrom(ETFactionId PlayerFaction) const { return Health->IsAlive() && Faction == PlayerFaction; }
void ATFUnit::SetAttackTarget(ATFUnit* Target) { if (HasAuthority() && Target && Target != this && Target->Faction != Faction) { CombatTarget = Target; CurrentRepairTarget = nullptr; } }
void ATFUnit::ClearCombatTarget() { if (HasAuthority()) CombatTarget = nullptr; }
bool ATFUnit::CanRepair(const ATFUnit* Target) const
{
    return Definition && Definition->RepairPerSecond > 0.f && Target && Target != this && Target->Faction == Faction && Target->Health->CurrentHealth < Target->Health->MaxHealth;
}
void ATFUnit::RepairTarget(ATFUnit* Target, float DeltaSeconds)
{
    if (HasAuthority() && CanRepair(Target)) Target->Health->Repair(Definition->RepairPerSecond * DeltaSeconds);
}
void ATFUnit::SetRepairTarget(ATFUnit* Target) { if (HasAuthority() && !Target) { CurrentRepairTarget = nullptr; } else if (HasAuthority() && CanRepair(Target)) { CurrentRepairTarget = Target; CombatTarget = nullptr; } }
void ATFUnit::SetFormationFacing(FVector Direction)
{
    Direction.Z = 0.f;
    if (HasAuthority() && !Direction.IsNearlyZero())
    {
        GetCharacterMovement()->bOrientRotationToMovement = false;
        SetActorRotation(Direction.Rotation());
    }
}
void ATFUnit::AutoAttack(float DeltaSeconds)
{
}
void ATFUnit::Tick(float DeltaSeconds) { Super::Tick(DeltaSeconds); AutoAttack(DeltaSeconds); if (CurrentRepairTarget) RepairTarget(CurrentRepairTarget, DeltaSeconds); }
void ATFUnit::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ATFUnit, Faction); DOREPLIFETIME(ATFUnit, Definition); DOREPLIFETIME(ATFUnit, CombatTarget); DOREPLIFETIME(ATFUnit, CurrentRepairTarget);
}
