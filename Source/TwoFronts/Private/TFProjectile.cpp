#include "TFProjectile.h"
#include "TFDamageSystem.h"
#include "TFUnit.h"
#include "TFHealthComponent.h"
#include "Components/StaticMeshComponent.h"

ATFProjectile::ATFProjectile()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);
    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
    SetRootComponent(Visual);
    Visual->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
    Visual->SetRelativeScale3D(FVector(.12f));
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
void ATFProjectile::Initialise(ATFUnit* InTarget, const FTFDamageRequest& InDamage, float InSpeed) { Target = InTarget; Damage = InDamage; Speed = FMath::Max(100.f, InSpeed); }
void ATFProjectile::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Lifetime -= DeltaSeconds;
    if (!HasAuthority() || Lifetime <= 0.f || !IsValid(Target) || !Target->Health || !Target->Health->IsAlive()) { Destroy(); return; }
    const FVector TargetLocation = Target->GetActorLocation() + FVector(0.f, 0.f, 50.f);
    const FVector Offset = TargetLocation - GetActorLocation();
    if (Offset.SizeSquared2D() <= FMath::Square(Speed * DeltaSeconds + 50.f)) { FTFDamageSystem::ApplyDamage(Target, Damage); Destroy(); return; }
    SetActorLocation(GetActorLocation() + Offset.GetSafeNormal() * Speed * DeltaSeconds);
}
