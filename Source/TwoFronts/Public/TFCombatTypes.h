#pragma once

#include "CoreMinimal.h"
#include "NiagaraSystem.h"
#include "TFCombatTypes.generated.h"

UENUM(BlueprintType)
enum class ETFCombatTargetCategory : uint8 { Light, Armored, Structure };

UENUM(BlueprintType)
enum class ETFWeaponDelivery : uint8 { Direct, Projectile, Ballistic };

UENUM(BlueprintType)
enum class ETFLaserVFXQuality : uint8 { Low, Medium, High };

// Presentation-only settings for an instantaneous Direct weapon. They never participate in target validation or damage.
USTRUCT(BlueprintType)
struct FTFDirectShotVFXSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Laser VFX") FLinearColor PrimaryColor = FLinearColor(1.f, .42f, .04f);
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Laser VFX") FLinearColor CoreColor = FLinearColor(1.f, .96f, .70f);
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Laser VFX", meta=(ClampMin="0.1")) float BeamWidth = 2.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Laser VFX", meta=(ClampMin="0.1")) float GlowWidth = 10.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Laser VFX", meta=(ClampMin="0.01")) float BeamLifetime = .09f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Laser VFX", meta=(ClampMin="0.0")) float MuzzleIntensity = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Laser VFX", meta=(ClampMin="0.0")) float ImpactIntensity = 1.35f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Laser VFX", meta=(ClampMin="0.0")) float LightIntensity = 18000.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Laser VFX", meta=(ClampMin="1.0")) float LightRadius = 240.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Laser VFX", meta=(ClampMin="0.01")) float LightLifetime = .12f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Laser VFX") bool bCastImpactShadows = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Laser VFX", meta=(ClampMin="0.0")) float VolumetricScatteringIntensity = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Laser VFX") ETFLaserVFXQuality VFXQuality = ETFLaserVFXQuality::High;

    // Optional, packaged Niagara systems. The fallback remains fully visible when no content assets are assigned.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Laser VFX|Niagara") TObjectPtr<UNiagaraSystem> BeamSystem = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Laser VFX|Niagara") TObjectPtr<UNiagaraSystem> MuzzleSystem = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Laser VFX|Niagara") TObjectPtr<UNiagaraSystem> ImpactSystem = nullptr;
};

USTRUCT(BlueprintType)
struct FTFDamageRequest
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float RawDamage = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bAllowFriendlyFire = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<AActor> SourceActor;
};
