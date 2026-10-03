#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StaticMesh.h"
#include "TFTypes.h"
#include "TFUnitDefinition.generated.h"

UCLASS(BlueprintType)
class TWOFRONTS_API UTFUnitDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity") FName UnitId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity") FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity") ETFactionId Faction = ETFactionId::None;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity") ETUnitRole Role = ETUnitRole::Combat;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats") float MaxHealth = 100.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats") float MoveSpeed = 500.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats") float SightRange = 1200.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat") FTFCombatStats Combat;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Support") float RepairPerSecond = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Production") float ProductionSeconds = 5.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual") TSoftObjectPtr<UStaticMesh> PlaceholderMesh;
};
