#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "TFTypes.h"
#include "TFPlayerState.generated.h"
UCLASS()
class TWOFRONTS_API ATFPlayerState : public APlayerState
{
    GENERATED_BODY()
public:
    UPROPERTY(Replicated, BlueprintReadOnly) ETFactionId ChosenFaction = ETFactionId::Humans;
};
