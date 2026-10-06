#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TFTypes.h"
#include "TFGameMode.generated.h"
class UTFFactionDefinition;
class UTFUnitDefinition;

UCLASS()
class TWOFRONTS_API ATFGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ATFGameMode();
    virtual void BeginPlay() override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    UFUNCTION(BlueprintCallable) UTFFactionDefinition* GetFactionDefinition(ETFactionId Faction) const;
    // Public so automation tests can verify the same live prototype catalog without an editor asset.
    void CreateRuntimeDefinitions();
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Test Arena", meta=(ClampMin="1", ClampMax="50")) int32 InitialUnitsPerCategory = 10;
    int32 GetInitialUnitsPerCategory() const { return InitialUnitsPerCategory; }
private:
    UPROPERTY(Transient) TObjectPtr<UTFFactionDefinition> Humans;
    UPROPERTY(Transient) TObjectPtr<UTFFactionDefinition> Synth;
    UTFUnitDefinition* MakeUnit(UTFFactionDefinition* FactionDefinition, FName Id, const FText& Name, ETUnitRole UnitRole, float HP, float Speed, float Damage, float Range, float Cooldown, float Repair, float BuildTime, const TCHAR* MeshPath);
    void BuildTestArena();
};
