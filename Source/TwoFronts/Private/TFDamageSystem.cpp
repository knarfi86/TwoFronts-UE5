#include "TFDamageSystem.h"
#include "TFUnit.h"
#include "TFHealthComponent.h"

float FTFDamageSystem::ApplyDamage(ATFUnit* Target, const FTFDamageRequest& Request)
{
    if (!IsValid(Target) || !Target->HasAuthority() || !Target->Health || Request.RawDamage <= 0.f) return 0.f;
    const ATFUnit* SourceUnit = Cast<ATFUnit>(Request.SourceActor);
    if (!Request.bAllowFriendlyFire && SourceUnit && SourceUnit->Faction == Target->Faction) return 0.f;
    const float FinalDamage = CalculateDamage(Request.RawDamage, Target->GetArmor());
    return Target->Health->ApplyDamage(FinalDamage);
}
float FTFDamageSystem::CalculateDamage(float RawDamage, float Armor) { return RawDamage <= 0.f ? 0.f : FMath::Max(1.f, RawDamage - FMath::Max(0.f, Armor)); }
