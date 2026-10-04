#pragma once

#include "CoreMinimal.h"
#include "TFCombatTypes.h"
#include "TFTypes.h"

class ATFUnit;

// The single authority for combat damage. Weapons never alter health directly.
class TWOFRONTS_API FTFDamageSystem
{
public:
    static float CalculateDamage(float RawDamage, float Armor);
    static bool CanApplyDamage(ETFactionId SourceFaction, ETFactionId TargetFaction, bool bAllowFriendlyFire);
    static float ApplyDamage(ATFUnit* Target, const FTFDamageRequest& Request);
};
