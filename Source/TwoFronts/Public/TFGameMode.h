#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TFTypes.h"
#include "TFCombatTypes.h"
#include "TFGameMode.generated.h"
class UTFFactionDefinition;
class UTFUnitDefinition;
class UTFWeaponDefinition;
class ATFUnit;

UCLASS()
class TWOFRONTS_API ATFGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ATFGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    UFUNCTION(BlueprintCallable) UTFFactionDefinition* GetFactionDefinition(ETFactionId Faction) const;
    // Public so automation tests can verify the same live prototype catalog without an editor asset.
    void CreateRuntimeDefinitions();
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Test Arena", meta=(ClampMin="1", ClampMax="50")) int32 InitialUnitsPerCategory = 10;
    int32 GetInitialUnitsPerCategory() const { return InitialUnitsPerCategory; }
    void StartCombatDemo();
    FString GetCombatDemoStatus() const;
    int32 GetCombatDemoRemaining(ETFactionId Faction) const;
private:
    UPROPERTY(Transient) TObjectPtr<UTFFactionDefinition> Humans;
    UPROPERTY(Transient) TObjectPtr<UTFFactionDefinition> Synth;
    UTFUnitDefinition* MakeUnit(UTFFactionDefinition* FactionDefinition, FName Id, const FText& Name, ETUnitRole UnitRole, float HP, float Speed, float Damage, float Range, float Cooldown, float Repair, float BuildTime, const TCHAR* MeshPath);
    UTFWeaponDefinition* MakeWeapon(FName Id, const FText& Name, float Damage, float Range, float Reload, ETFWeaponDelivery Delivery, float ProjectileSpeed = 1800.f);
    void BuildTestArena();
    ATFUnit* SpawnCombatDemoUnit(UTFUnitDefinition* Definition, const FVector& Location);
    void UpdateCombatDemo();
    UPROPERTY(Transient) TArray<TObjectPtr<UTFWeaponDefinition>> RuntimeWeapons;
    UPROPERTY(Transient) TArray<TObjectPtr<ATFUnit>> CombatDemoHumans;
    UPROPERTY(Transient) TArray<TObjectPtr<ATFUnit>> CombatDemoSynth;
    float CombatDemoStartTime = -1.f;
    bool bCombatDemoStarted = false;
    bool bCombatDemoFinished = false;
};
