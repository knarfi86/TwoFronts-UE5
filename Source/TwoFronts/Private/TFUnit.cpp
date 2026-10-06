#include "TFUnit.h"
#include "TFHealthComponent.h"
#include "AIController.h"
#include "TFUnitDefinition.h"
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
    const float Applied = Health ? Health->ApplyDamage(DamageAmount) : 0.f;
    Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    return Applied;
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
    if (!HasAuthority() || !Definition) return;
    if (!CombatTarget || !CombatTarget->Health->IsAlive())
    {
        CombatTarget = nullptr;
        TArray<AActor*> Candidates; UGameplayStatics::GetAllActorsOfClass(this, ATFUnit::StaticClass(), Candidates);
        float BestDistance = Definition->SightRange;
        for (AActor* Candidate : Candidates)
        {
            ATFUnit* Other = Cast<ATFUnit>(Candidate);
            const float Distance = Other ? FVector::Dist2D(GetActorLocation(), Other->GetActorLocation()) : TNumericLimits<float>::Max();
            if (Other && Other->Faction != Faction && Other->Health->IsAlive() && Distance < BestDistance) { BestDistance = Distance; CombatTarget = Other; }
        }
    }
    if (!CombatTarget) return;
    const float Distance = FVector::Dist2D(GetActorLocation(), CombatTarget->GetActorLocation());
    if (Distance > Definition->Combat.Range) return;
    const float Now = GetWorld()->GetTimeSeconds();
    if (Now - LastAttackTime >= Definition->Combat.Cooldown && Definition->Combat.Damage > 0.f)
    {
        LastAttackTime = Now;
        UGameplayStatics::ApplyDamage(CombatTarget, Definition->Combat.Damage, GetController(), this, nullptr);
    }
}
void ATFUnit::Tick(float DeltaSeconds) { Super::Tick(DeltaSeconds); AutoAttack(DeltaSeconds); if (CurrentRepairTarget) RepairTarget(CurrentRepairTarget, DeltaSeconds); }
void ATFUnit::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ATFUnit, Faction); DOREPLIFETIME(ATFUnit, Definition); DOREPLIFETIME(ATFUnit, CombatTarget); DOREPLIFETIME(ATFUnit, CurrentRepairTarget);
}
