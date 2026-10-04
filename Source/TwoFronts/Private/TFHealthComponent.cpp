#include "TFHealthComponent.h"
#include "Net/UnrealNetwork.h"

UTFHealthComponent::UTFHealthComponent()
{
    SetIsReplicatedByDefault(true);
    MaxHealth = 100.f;
    CurrentHealth = MaxHealth;
}
void UTFHealthComponent::Initialise(float InMaxHealth)
{
    MaxHealth = FMath::Max(1.f, InMaxHealth);
    CurrentHealth = MaxHealth;
    OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}
float UTFHealthComponent::ApplyDamage(float Damage)
{
    if (Damage <= 0.f || !IsAlive()) return 0.f;
    const float Applied = FMath::Min(Damage, CurrentHealth);
    CurrentHealth -= Applied;
    OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
    if (!IsAlive() && GetOwner()) GetOwner()->Destroy();
    return Applied;
}
float UTFHealthComponent::Repair(float Amount)
{
    if (Amount <= 0.f || !IsAlive()) return 0.f;
    const float Before = CurrentHealth;
    CurrentHealth = FMath::Min(MaxHealth, CurrentHealth + Amount);
    if (CurrentHealth != Before) OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
    return CurrentHealth - Before;
}
void UTFHealthComponent::OnRep_Health() { OnHealthChanged.Broadcast(CurrentHealth, MaxHealth); }
void UTFHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UTFHealthComponent, CurrentHealth);
}
