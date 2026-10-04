#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TFTypes.h"
#include "TFUnit.generated.h"
class UTFHealthComponent;
class UTFUnitDefinition;
class UStaticMeshComponent;
class UPointLightComponent;
class UTextRenderComponent;
class UTFCombatComponent;

UCLASS(Blueprintable)
class TWOFRONTS_API ATFUnit : public ACharacter
{
    GENERATED_BODY()
public:
    ATFUnit();
    virtual void Tick(float DeltaSeconds) override;
    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Visual;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> SelectionMarker;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> HealthBar;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UPointLightComponent> FactionLight;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UTextRenderComponent> FactionMarker;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UTFHealthComponent> Health;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UTFCombatComponent> CombatController;
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly) ETFactionId Faction;
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UTFUnitDefinition> Definition;
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly) TObjectPtr<ATFUnit> CombatTarget;
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly) TObjectPtr<ATFUnit> CurrentRepairTarget;
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly) bool bCombatEnabled = true;
    UFUNCTION(BlueprintCallable) void ApplyDefinition(UTFUnitDefinition* InDefinition);
    UFUNCTION(BlueprintCallable) void SetSelectedVisual(bool bSelected);
    UFUNCTION(BlueprintCallable) bool CanReceiveOrdersFrom(ETFactionId PlayerFaction) const;
    UFUNCTION(BlueprintCallable) void SetAttackTarget(ATFUnit* Target);
    UFUNCTION(BlueprintCallable) void ClearCombatTarget();
    UFUNCTION(BlueprintCallable) bool CanRepair(const ATFUnit* Target) const;
    UFUNCTION(BlueprintCallable) void RepairTarget(ATFUnit* Target, float DeltaSeconds);
    UFUNCTION(BlueprintCallable) void SetRepairTarget(ATFUnit* Target);
    UFUNCTION(BlueprintCallable) void SetFormationFacing(FVector Direction);
    UFUNCTION(BlueprintCallable) float GetArmor() const;
    UFUNCTION() void UpdateHealthVisual(float CurrentHealth, float MaximumHealth);
protected:
    float LastAttackTime = -100.f;
    void AutoAttack(float DeltaSeconds);
};
