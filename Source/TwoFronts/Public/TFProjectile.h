#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TFCombatTypes.h"
#include "TFProjectile.generated.h"

class UStaticMeshComponent;
class ATFUnit;

UCLASS()
class TWOFRONTS_API ATFProjectile : public AActor
{
    GENERATED_BODY()
public:
    ATFProjectile();
    virtual void Tick(float DeltaSeconds) override;
    void Initialise(ATFUnit* InTarget, const FTFDamageRequest& InDamage, float InSpeed);
private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Visual;
    UPROPERTY() TObjectPtr<ATFUnit> Target;
    FTFDamageRequest Damage;
    float Speed = 1800.f;
    float Lifetime = 5.f;
};
