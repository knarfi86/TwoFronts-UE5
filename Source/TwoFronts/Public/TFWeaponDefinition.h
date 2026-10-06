#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TFCombatTypes.h"
#include "TFWeaponDefinition.generated.h"

UCLASS(BlueprintType)
class TWOFRONTS_API UTFWeaponDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity") FName WeaponId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity") FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage") float Damage = 10.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Range") float MinimumRange = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Range") float MaximumRange = 600.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fire") float ReloadSeconds = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fire") float ProjectileSpeed = 1800.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fire") float Accuracy = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Damage") float AreaDamageRadius = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Delivery") ETFWeaponDelivery Delivery = ETFWeaponDelivery::Direct;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Targets") TArray<ETFCombatTargetCategory> ValidTargetCategories;
    // Read only by the presentation layer after an already-resolved Direct hit.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation") FTFDirectShotVFXSettings DirectShotVFX;
};
