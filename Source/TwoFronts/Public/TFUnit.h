#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TFTypes.h"
#include "TFUnit.generated.h"
class UTFHealthComponent;
class UTFUnitDefinition;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class TWOFRONTS_API ATFUnit : public ACharacter
{
    GENERATED_BODY()
public:
    ATFUnit();
    virtual void Tick(float DeltaSeconds) override;
    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Visual;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> SelectionMarker;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UTFHealthComponent> Health;
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly) ETFactionId Faction;
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UTFUnitDefinition> Definition;
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly) TObjectPtr<ATFUnit> CombatTarget;
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly) TObjectPtr<ATFUnit> CurrentRepairTarget;
    UFUNCTION(BlueprintCallable) void ApplyDefinition(UTFUnitDefinition* InDefinition);
    UFUNCTION(BlueprintCallable) void SetSelectedVisual(bool bSelected);
    UFUNCTION(BlueprintCallable) bool CanReceiveOrdersFrom(ETFactionId PlayerFaction) const;
    UFUNCTION(BlueprintCallable) void SetAttackTarget(ATFUnit* Target);
    UFUNCTION(BlueprintCallable) void ClearCombatTarget();
    UFUNCTION(BlueprintCallable) bool CanRepair(const ATFUnit* Target) const;
    UFUNCTION(BlueprintCallable) void RepairTarget(ATFUnit* Target, float DeltaSeconds);
    UFUNCTION(BlueprintCallable) void SetRepairTarget(ATFUnit* Target);
protected:
    float LastAttackTime = -100.f;
    void AutoAttack(float DeltaSeconds);
};
