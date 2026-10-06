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

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blaster Bolt|Color") FLinearColor BoltColor = FLinearColor(1.f, .42f, .04f);
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blaster Bolt|Color") FLinearColor CoreColor = FLinearColor(1.f, .96f, .70f);
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blaster Bolt|Color") FLinearColor GlowColor = FLinearColor(1.f, .20f, .01f);
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blaster Bolt|Color") FLinearColor MuzzleColor = FLinearColor(1.f, .55f, .08f);
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blaster Bolt|Color") FLinearColor ImpactColor = FLinearColor(1.f, .32f, .02f);
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blaster Bolt", meta=(ClampMin="80.0", ClampMax="250.0")) float BoltLength = 150.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blaster Bolt", meta=(ClampMin="4.0", ClampMax="15.0")) float BoltWidth = 7.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blaster Bolt", meta=(ClampMin="4.0", ClampMax="30.0")) float GlowWidth = 13.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blaster Bolt", meta=(ClampMin="15000.0", ClampMax="40000.0")) float BoltSpeed = 26000.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blaster Bolt", meta=(ClampMin="0.01", ClampMax="0.25")) float BoltLifetime = .10f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blaster Bolt", meta=(ClampMin="0.0", ClampMax="75.0")) float TrailLength = 24.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blaster Bolt", meta=(ClampMin="0.0", ClampMax="15.0")) float TrailWidth = 3.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blaster Bolt", meta=(ClampMin="0.0", ClampMax="12.0")) float BoltJitter = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Muzzle", meta=(ClampMin="0.1")) float MuzzleScale = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Muzzle", meta=(ClampMin="0.01", ClampMax="0.2")) float MuzzleLifetime = .04f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Impact", meta=(ClampMin="0.1")) float ImpactScale = 1.35f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Impact", meta=(ClampMin="0.01", ClampMax="0.25")) float ImpactLifetime = .07f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lighting", meta=(ClampMin="0.0")) float LightIntensity = 18000.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lighting", meta=(ClampMin="1.0")) float LightRadius = 240.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lighting", meta=(ClampMin="0.01", ClampMax="0.2")) float LightLifetime = .06f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lighting") bool bCastImpactShadows = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lighting", meta=(ClampMin="0.0")) float VolumetricScatteringIntensity = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quality") ETFLaserVFXQuality VFXQuality = ETFLaserVFXQuality::High;

    // Optional, packaged Niagara systems. BoltSystem must render one short moving bolt, never a source-to-target beam.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blaster Bolt|Niagara") TObjectPtr<UNiagaraSystem> BoltSystem = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blaster Bolt|Niagara") TObjectPtr<UNiagaraSystem> MuzzleSystem = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blaster Bolt|Niagara") TObjectPtr<UNiagaraSystem> ImpactSystem = nullptr;
};

USTRUCT(BlueprintType)
struct FTFDamageRequest
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float RawDamage = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bAllowFriendlyFire = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<AActor> SourceActor;
};
