#include "TFCombatComponent.h"
#include "TFUnit.h"
#include "TFUnitDefinition.h"
#include "TFHealthComponent.h"
#include "TFWeaponDefinition.h"
#include "TFDamageSystem.h"
#include "TFProjectile.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"

UTFCombatComponent::UTFCombatComponent() { PrimaryComponentTick.bCanEverTick = true; SetIsReplicatedByDefault(true); }
bool UTFCombatComponent::IsValidTarget(const ATFUnit* OwnerUnit, const ATFUnit* Target) const
{
    if (!OwnerUnit || !Target || OwnerUnit == Target || !OwnerUnit->Definition || !Target->Definition || OwnerUnit->Faction == Target->Faction || !Target->Health->IsAlive()) return false;
    for (const UTFWeaponDefinition* Weapon : OwnerUnit->Definition->Weapons) if (Weapon && Weapon->ValidTargetCategories.Contains(Target->Definition->TargetCategory)) return true;
    return false;
}
ATFUnit* UTFCombatComponent::FindBestTarget(ATFUnit* OwnerUnit) const
{
    ATFUnit* Best = nullptr; float BestDistance = TNumericLimits<float>::Max();
    for (TActorIterator<ATFUnit> It(GetWorld()); It; ++It)
    {
        const float Distance = FVector::Dist2D(OwnerUnit->GetActorLocation(), It->GetActorLocation());
        if (Distance <= OwnerUnit->Definition->AcquisitionRange && IsValidTarget(OwnerUnit, *It) && Distance < BestDistance) { Best = *It; BestDistance = Distance; }
    }
    return Best;
}
void UTFCombatComponent::FireWeapon(ATFUnit* OwnerUnit, int32 WeaponIndex)
{
    const UTFWeaponDefinition* Weapon = OwnerUnit->Definition->Weapons[WeaponIndex];
    ATFUnit* Target = OwnerUnit->CombatTarget;
    FTFDamageRequest Damage; Damage.RawDamage = Weapon->Damage; Damage.SourceActor = OwnerUnit;
    if (Weapon->Delivery == ETFWeaponDelivery::Projectile)
    {
        if (ATFProjectile* Projectile = GetWorld()->SpawnActor<ATFProjectile>(OwnerUnit->GetActorLocation() + FVector(0.f, 0.f, 70.f), FRotator::ZeroRotator)) Projectile->Initialise(Target, Damage, Weapon->ProjectileSpeed);
    }
    else { DrawDebugLine(GetWorld(), OwnerUnit->GetActorLocation() + FVector(0.f,0.f,70.f), Target->GetActorLocation() + FVector(0.f,0.f,70.f), FColor::Yellow, false, .08f, 0, 2.f); FTFDamageSystem::ApplyDamage(Target, Damage); }
}
void UTFCombatComponent::TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);
    ATFUnit* OwnerUnit = Cast<ATFUnit>(GetOwner());
    if (!OwnerUnit || !OwnerUnit->HasAuthority() || !OwnerUnit->Definition || !OwnerUnit->Health->IsAlive()) return;
    if (!IsValidTarget(OwnerUnit, OwnerUnit->CombatTarget) || FVector::Dist2D(OwnerUnit->GetActorLocation(), OwnerUnit->CombatTarget->GetActorLocation()) > OwnerUnit->Definition->AcquisitionRange) OwnerUnit->CombatTarget = FindBestTarget(OwnerUnit);
    if (!OwnerUnit->CombatTarget) return;
    if (NextFireTimes.Num() != OwnerUnit->Definition->Weapons.Num()) NextFireTimes.SetNumZeroed(OwnerUnit->Definition->Weapons.Num());
    const float Now = GetWorld()->GetTimeSeconds();
    const float Distance = FVector::Dist2D(OwnerUnit->GetActorLocation(), OwnerUnit->CombatTarget->GetActorLocation());
    for (int32 Index = 0; Index < OwnerUnit->Definition->Weapons.Num(); ++Index)
    {
        const UTFWeaponDefinition* Weapon = OwnerUnit->Definition->Weapons[Index];
        if (Weapon && Distance >= Weapon->MinimumRange && Distance <= Weapon->MaximumRange && Now >= NextFireTimes[Index]) { FireWeapon(OwnerUnit, Index); NextFireTimes[Index] = Now + FMath::Max(.05f, Weapon->ReloadSeconds); }
    }
}
