#include "TFProjectile.h"
#include "TFDamageSystem.h"
#include "TFUnit.h"
#include "TFHealthComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

ATFProjectile::ATFProjectile()
{
    bReplicates = true;
    SetReplicateMovement(true);
    InitialLifeSpan = 5.f;
    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    Collision->SetSphereRadius(12.f);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Collision->SetCollisionObjectType(ECC_WorldDynamic);
    Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
    Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    SetRootComponent(Collision);
    Collision->OnComponentBeginOverlap.AddDynamic(this, &ATFProjectile::HandleProjectileOverlap);
    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
    Visual->SetupAttachment(Collision);
    Visual->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
    Visual->SetRelativeScale3D(FVector(.12f));
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->UpdatedComponent = Collision;
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->bIsHomingProjectile = true;
}
void ATFProjectile::Initialise(ATFUnit* InTarget, const FTFDamageRequest& InDamage, float InSpeed)
{
    if (!HasAuthority() || !IsValid(InTarget) || !InTarget->GetRootComponent()) { Destroy(); return; }
    Target = InTarget;
    Damage = InDamage;
    const float ProjectileSpeed = FMath::Max(100.f, InSpeed);
    ProjectileMovement->InitialSpeed = ProjectileSpeed;
    ProjectileMovement->MaxSpeed = ProjectileSpeed;
    ProjectileMovement->HomingAccelerationMagnitude = ProjectileSpeed * 4.f;
    ProjectileMovement->HomingTargetComponent = Target->GetRootComponent();
    ProjectileMovement->Velocity = (Target->GetActorLocation() + FVector(0.f, 0.f, 50.f) - GetActorLocation()).GetSafeNormal() * ProjectileSpeed;
    ProjectileMovement->Activate(true);
}
void ATFProjectile::HandleProjectileOverlap(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
    if (!HasAuthority() || OtherActor != Target) return;
    if (Target && Target->Health && Target->Health->IsAlive()) FTFDamageSystem::ApplyDamage(Target, Damage);
    Destroy();
}
