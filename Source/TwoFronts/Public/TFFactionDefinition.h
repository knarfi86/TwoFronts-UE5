#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TFTypes.h"
#include "TFFactionDefinition.generated.h"
class UTFUnitDefinition;

UCLASS(BlueprintType)
class TWOFRONTS_API UTFFactionDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) ETFactionId Faction = ETFactionId::None;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FLinearColor TeamColor = FLinearColor::White;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<TObjectPtr<UTFUnitDefinition>> Units;
};
