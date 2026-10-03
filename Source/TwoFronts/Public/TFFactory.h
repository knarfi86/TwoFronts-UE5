#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TFTypes.h"
#include "TFFactory.generated.h"
class UTFHealthComponent;
class UTFUnitDefinition;
class ATFUnit;
class UStaticMeshComponent;
class UBoxComponent;

USTRUCT(BlueprintType)
struct FTFProductionItem
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) TObjectPtr<UTFUnitDefinition> UnitDefinition;
    UPROPERTY(BlueprintReadOnly) float RemainingSeconds = 0.f;
};

UCLASS(Blueprintable)
class TWOFRONTS_API ATFFactory : public AActor
{
    GENERATED_BODY()
public:
    ATFFactory();
    virtual void Tick(float DeltaSeconds) override;
    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Visual;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UBoxComponent> Collision;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UTFHealthComponent> Health;
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly) ETFactionId Faction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<TObjectPtr<UTFUnitDefinition>> ProductionOptions;
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly) TArray<FTFProductionItem> ProductionQueue;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector SpawnOffset = FVector(750.f, 0.f, 0.f);
    UFUNCTION(BlueprintCallable) bool EnqueueProduction(int32 OptionIndex);
    UFUNCTION(BlueprintCallable) bool CanReceiveOrdersFrom(ETFactionId PlayerFaction) const { return Faction == PlayerFaction; }
private:
    void SpawnCompletedUnit(UTFUnitDefinition* UnitDefinition);
};
