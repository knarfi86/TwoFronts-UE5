#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TFCombatTypes.h"
#include "TFLaserVFX.generated.h"

class UPointLightComponent;
class USceneComponent;
class UNiagaraSystem;

// A short-lived, client-side presentation actor. It contains no combat, trace, target, or damage logic.
UCLASS(NotBlueprintable)
class TWOFRONTS_API ATFLaserVFX : public AActor
{
    GENERATED_BODY()
public:
    ATFLaserVFX();
    static void Spawn(UWorld* World, const FVector& MuzzleLocation, const FVector& ImpactLocation, const FTFDirectShotVFXSettings& Settings);
    void Initialise(const FVector& MuzzleLocation, const FVector& ImpactLocation, const FTFDirectShotVFXSettings& Settings);

private:
    void DrawFallbackBeam(float WidthScale = 1.f) const;
    void DrawFallbackMuzzle() const;
    void DrawFallbackImpact() const;
    void PulseFallbackBeam();
    bool SpawnNiagaraSystem(UNiagaraSystem* System, const FVector& Location, const FRotator& Rotation, bool bConfigureBeam);

    UPROPERTY(VisibleAnywhere, Category="Laser VFX") TObjectPtr<USceneComponent> Root;
    UPROPERTY(VisibleAnywhere, Category="Laser VFX") TObjectPtr<UPointLightComponent> MuzzleLight;
    UPROPERTY(VisibleAnywhere, Category="Laser VFX") TObjectPtr<UPointLightComponent> ImpactLight;
    FTFDirectShotVFXSettings VFXSettings;
    FVector BeamStart = FVector::ZeroVector;
    FVector BeamEnd = FVector::ZeroVector;
    FTimerHandle FallbackPulseTimer;
    int32 PulseCount = 0;
};
