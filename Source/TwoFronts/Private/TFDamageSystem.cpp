#include "TFDamageSystem.h"
#include "TFUnit.h"
#include "TFHealthComponent.h"

float FTFDamageSystem::ApplyDamage(ATFUnit* Target, const FTFDamageRequest& Request)
{
    if (!IsValid(Target) || !Target->HasAuthority() || !Target->Health || Request.RawDamage <= 0.f) return 0.f;
    const ATFUnit* SourceUnit = Cast<ATFUnit>(Request.SourceActor);
    if (SourceUnit && !CanApplyDamage(SourceUnit->Faction, Target->Faction, Request.bAllowFriendlyFire)) return 0.f;
    const float FinalDamage = CalculateDamage(Request.RawDamage, Target->GetArmor());
    const float Applied = Target->Health->ApplyDamage(FinalDamage);
    if (Applied > 0.f) UE_LOG(LogTemp, Display, TEXT("COMBAT_DAMAGE %s took %.1f from %s; HP %.1f"), *Target->GetName(), Applied, Request.SourceActor ? *Request.SourceActor->GetName() : TEXT("unknown"), Target->Health->CurrentHealth);
    return Applied;
}
float FTFDamageSystem::CalculateDamage(float RawDamage, float Armor) { return RawDamage <= 0.f ? 0.f : FMath::Max(1.f, RawDamage - FMath::Max(0.f, Armor)); }
bool FTFDamageSystem::CanApplyDamage(ETFactionId SourceFaction, ETFactionId TargetFaction, bool bAllowFriendlyFire) { return bAllowFriendlyFire || SourceFaction != TargetFaction; }
