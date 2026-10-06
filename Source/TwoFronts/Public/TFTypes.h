#pragma once
#include "CoreMinimal.h"
#include "TFTypes.generated.h"

UENUM(BlueprintType)
enum class ETFactionId : uint8 { None, Humans, Synth };

UENUM(BlueprintType)
enum class ETUnitRole : uint8 { Scout, Combat, Heavy, Support };

UENUM(BlueprintType)
enum class ETFFormation : uint8 { Line, Column, Wedge };

USTRUCT(BlueprintType)
struct FTFCombatStats
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float Damage = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float Range = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float Cooldown = 1.f;
};
