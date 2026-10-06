#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TFCombatTypes.h"
#include "TFCombatComponent.generated.h"

class ATFUnit;

UCLASS(ClassGroup=(TwoFronts), meta=(BlueprintSpawnableComponent))
class TWOFRONTS_API UTFCombatComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UTFCombatComponent();
    virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
private:
    ATFUnit* FindBestTarget(ATFUnit* OwnerUnit) const;
    bool IsValidTarget(const ATFUnit* OwnerUnit, const ATFUnit* Target) const;
    void FireWeapon(ATFUnit* OwnerUnit, int32 WeaponIndex);
    UFUNCTION(NetMulticast, Unreliable)
    void MulticastShowDirectShotVFX(FVector_NetQuantize10 MuzzleLocation, FVector_NetQuantize10 ImpactLocation, const FTFDirectShotVFXSettings& VFXSettings);
    TArray<float> NextFireTimes;
    bool bLoggedFirstShot = false;
};
