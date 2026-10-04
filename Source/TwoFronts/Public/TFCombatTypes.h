#pragma once

#include "CoreMinimal.h"
#include "TFCombatTypes.generated.h"

UENUM(BlueprintType)
enum class ETFCombatTargetCategory : uint8 { Light, Armored, Structure };

UENUM(BlueprintType)
enum class ETFWeaponDelivery : uint8 { Direct, Projectile, Ballistic };

USTRUCT(BlueprintType)
struct FTFDamageRequest
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float RawDamage = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bAllowFriendlyFire = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<AActor> SourceActor;
};
