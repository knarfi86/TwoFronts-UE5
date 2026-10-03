#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TFHealthComponent.generated.h"
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTFHealthChanged, float, Current, float, Maximum);

UCLASS(ClassGroup=(TwoFronts), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class TWOFRONTS_API UTFHealthComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UTFHealthComponent();
    UPROPERTY(ReplicatedUsing=OnRep_Health, VisibleAnywhere, BlueprintReadOnly) float CurrentHealth;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float MaxHealth;
    UPROPERTY(BlueprintAssignable) FTFHealthChanged OnHealthChanged;
    UFUNCTION(BlueprintCallable) bool IsAlive() const { return CurrentHealth > 0.f; }
    UFUNCTION(BlueprintCallable) float GetHealthPercent() const { return MaxHealth > 0.f ? CurrentHealth / MaxHealth : 0.f; }
    void Initialise(float InMaxHealth);
    float ApplyDamage(float Amount);
    float Repair(float Amount);
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
protected:
    UFUNCTION() void OnRep_Health();
};
