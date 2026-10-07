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

    // Authored maps keep their own geometry. Navigation bounds are generated automatically from loaded Landscape actors.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Navigation") bool bAutoCreateNavigationForAuthoredMaps = true;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Navigation", meta=(ClampMin="0.0")) float AuthoredMapNavigationPaddingXY = 1500.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Navigation", meta=(ClampMin="0.0")) float AuthoredMapNavigationPaddingZ = 2000.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Navigation", meta=(ClampMin="100.0")) float AuthoredMapNavigationMinHalfHeight = 3000.f;

    // Real maps automatically receive their initial factions at opposite corners.
    // World +X/+Y is treated as northeast; -X/-Y as southwest.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Authored Map Spawning") bool bAutoSpawnForcesOnAuthoredMaps = true;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Authored Map Spawning", meta=(ClampMin="0.20", ClampMax="0.90")) float AuthoredMapBaseCornerFraction = 0.68f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Authored Map Spawning", meta=(ClampMin="100.0")) float AuthoredMapBaseSearchRadiusXY = 5000.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Authored Map Spawning", meta=(ClampMin="500.0")) float AuthoredMapSpawnProjectionExtentZ = 12000.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Authored Map Spawning", meta=(ClampMin="100.0")) float AuthoredMapUnitProjectionRadiusXY = 900.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Authored Map Spawning", meta=(ClampMin="100.0")) float AuthoredMapFactoryToArmyDistance = 1100.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Authored Map Spawning", meta=(ClampMin="100.0")) float AuthoredMapArmyRowSpacing = 380.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Authored Map Spawning", meta=(ClampMin="100.0")) float AuthoredMapArmyUnitSpacing = 320.f;
    void StartCombatDemo();
    FString GetCombatDemoStatus() const;
    int32 GetCombatDemoRemaining(ETFactionId Faction) const;
private:
    UPROPERTY(Transient) TObjectPtr<UTFFactionDefinition> Humans;
    UPROPERTY(Transient) TObjectPtr<UTFFactionDefinition> Synth;
    UTFUnitDefinition* MakeUnit(UTFFactionDefinition* FactionDefinition, FName Id, const FText& Name, ETUnitRole UnitRole, float HP, float Speed, float Damage, float Range, float Cooldown, float Repair, float BuildTime, const TCHAR* MeshPath);
    UTFWeaponDefinition* MakeWeapon(FName Id, const FText& Name, float Damage, float Range, float Reload, ETFWeaponDelivery Delivery, float ProjectileSpeed = 1800.f);
    void BuildTestArena();
    void BuildAuthoredMapNavigation();
    void BuildAuthoredMapForces();
    bool GetAuthoredLandscapeBounds(FBox& OutBounds, int32& OutLandscapeActorCount) const;
    bool FindNavigableAuthoredMapPoint(const FVector& Candidate, FVector& OutLocation, float SearchXY) const;
    bool FindAuthoredMapBasePoint(const FBox& LandscapeBounds, bool bNorthEast, FVector& OutLocation) const;
    ATFUnit* SpawnCombatDemoUnit(UTFUnitDefinition* Definition, const FVector& Location);
    void UpdateCombatDemo();
    UPROPERTY(Transient) TArray<TObjectPtr<UTFWeaponDefinition>> RuntimeWeapons;
    UPROPERTY(Transient) TArray<TObjectPtr<ATFUnit>> CombatDemoHumans;
    UPROPERTY(Transient) TArray<TObjectPtr<ATFUnit>> CombatDemoSynth;
    float CombatDemoStartTime = -1.f;
    bool bCombatDemoStarted = false;
    bool bCombatDemoFinished = false;
    bool bAuthoredMapForcesSpawned = false;
};
