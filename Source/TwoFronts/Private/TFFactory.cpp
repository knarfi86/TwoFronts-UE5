#include "TFFactory.h"
#include "TFHealthComponent.h"
#include "TFUnit.h"
#include "TFUnitDefinition.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"

ATFFactory::ATFFactory()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true; SetReplicateMovement(true);
    Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
    Collision->SetBoxExtent(FVector(450.f, 450.f, 250.f)); SetRootComponent(Collision);
    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual")); Visual->SetupAttachment(Collision); Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Health = CreateDefaultSubobject<UTFHealthComponent>(TEXT("Health")); Health->Initialise(1800.f);
}
bool ATFFactory::EnqueueProduction(int32 OptionIndex)
{
    if (!HasAuthority() || !ProductionOptions.IsValidIndex(OptionIndex) || !ProductionOptions[OptionIndex]) return false;
    UTFUnitDefinition* Definition = ProductionOptions[OptionIndex];
    if (Definition->Faction != Faction) return false;
    FTFProductionItem& Item = ProductionQueue.AddDefaulted_GetRef();
    Item.UnitDefinition = Definition; Item.RemainingSeconds = FMath::Max(.1f, Definition->ProductionSeconds);
    return true;
}
float ATFFactory::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    const float Applied = Health ? Health->ApplyDamage(DamageAmount) : 0.f;
    Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
    return Applied;
}
void ATFFactory::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!HasAuthority() || ProductionQueue.IsEmpty()) return;
    FTFProductionItem& Current = ProductionQueue[0]; Current.RemainingSeconds -= DeltaSeconds;
    if (Current.RemainingSeconds <= 0.f) { SpawnCompletedUnit(Current.UnitDefinition); ProductionQueue.RemoveAt(0); }
}
void ATFFactory::SpawnCompletedUnit(UTFUnitDefinition* UnitDefinition)
{
    if (!UnitDefinition) return;
    FVector Location = GetActorLocation() + GetActorRotation().RotateVector(SpawnOffset);
    FActorSpawnParameters Params; Params.Owner = this;
    if (ATFUnit* Unit = GetWorld()->SpawnActor<ATFUnit>(ATFUnit::StaticClass(), Location, GetActorRotation(), Params))
    {
        Unit->ApplyDefinition(UnitDefinition);
        if (UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld())) { FNavLocation NavLocation; if (Nav->ProjectPointToNavigation(Location, NavLocation)) Unit->SetActorLocation(NavLocation.Location); }
    }
}
void ATFFactory::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const
{
    Super::GetLifetimeReplicatedProps(Out);
    DOREPLIFETIME(ATFFactory, Faction); DOREPLIFETIME(ATFFactory, ProductionQueue);
}
