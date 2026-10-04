#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TFCombatTypes.h"
#include "TFProjectile.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class UProjectileMovementComponent;
class ATFUnit;

UCLASS()
class TWOFRONTS_API ATFProjectile : public AActor
{
    GENERATED_BODY()
public:
    ATFProjectile();
    void Initialise(ATFUnit* InTarget, const FTFDamageRequest& InDamage, float InSpeed);
private:
    UFUNCTION()
    void HandleProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
    UPROPERTY() TObjectPtr<USphereComponent> Collision;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Visual;
    UPROPERTY() TObjectPtr<UProjectileMovementComponent> ProjectileMovement;
    UPROPERTY() TObjectPtr<ATFUnit> Target;
    FTFDamageRequest Damage;
};
