#include "TFLaserVFX.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

ATFLaserVFX::ATFLaserVFX()
{
    PrimaryActorTick.bCanEverTick = false;
    SetReplicates(false);
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    MuzzleLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MuzzleLight"));
    MuzzleLight->SetupAttachment(Root);
    MuzzleLight->SetCastShadows(false);
    MuzzleLight->SetVisibility(false);
    ImpactLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ImpactLight"));
    ImpactLight->SetupAttachment(Root);
    ImpactLight->SetVisibility(false);
}

void ATFLaserVFX::Spawn(UWorld* World, const FVector& MuzzleLocation, const FVector& ImpactLocation, const FTFDirectShotVFXSettings& Settings)
{
    if (!World || Settings.BoltLifetime <= 0.f) return;
    if (ATFLaserVFX* Effect = World->SpawnActor<ATFLaserVFX>(MuzzleLocation, FRotator::ZeroRotator)) Effect->Initialise(MuzzleLocation, ImpactLocation, Settings);
}

void ATFLaserVFX::Initialise(const FVector& MuzzleLocation, const FVector& ImpactLocation, const FTFDirectShotVFXSettings& Settings)
{
    VFXSettings = Settings;
    BoltStart = MuzzleLocation;
    BoltEnd = ImpactLocation;
    BoltDirection = (BoltEnd - BoltStart).GetSafeNormal();
    const float BoltDistance = FVector::Distance(BoltStart, BoltEnd);
    BoltTravelDuration = FMath::Clamp(BoltDistance / FMath::Max(15000.f, VFXSettings.BoltSpeed), .005f, VFXSettings.BoltLifetime);
    BoltSpawnTime = GetWorld()->GetTimeSeconds();
    SetActorLocation(BoltStart);
    InitialLifeSpan = FMath::Max(.01f, BoltTravelDuration + FMath::Max(VFXSettings.ImpactLifetime, VFXSettings.VFXQuality == ETFLaserVFXQuality::High ? VFXSettings.LightLifetime : 0.f) + .01f);

    const FRotator BoltRotation = BoltDirection.Rotation();
    if (!SpawnNiagaraSystem(VFXSettings.BoltSystem, BoltStart, BoltRotation, 1.f, true))
    {
        DrawFallbackBolt();
        const float StepInterval = FMath::Clamp(BoltTravelDuration / 4.f, .005f, .012f);
        GetWorldTimerManager().SetTimer(BoltStepTimer, this, &ATFLaserVFX::AdvanceFallbackBolt, StepInterval, true, StepInterval);
    }
    if (!SpawnNiagaraSystem(VFXSettings.MuzzleSystem, BoltStart, BoltRotation, VFXSettings.MuzzleScale, false)) DrawFallbackMuzzle();
    GetWorldTimerManager().SetTimer(ImpactTimer, this, &ATFLaserVFX::PlayImpact, BoltTravelDuration, false);
    if (VFXSettings.VFXQuality == ETFLaserVFXQuality::High)
    {
        MuzzleLight->SetLightColor(VFXSettings.MuzzleColor);
        MuzzleLight->SetIntensity(VFXSettings.LightIntensity * VFXSettings.MuzzleScale);
        MuzzleLight->SetAttenuationRadius(VFXSettings.LightRadius * .65f);
        MuzzleLight->SetVolumetricScatteringIntensity(VFXSettings.VolumetricScatteringIntensity);
        MuzzleLight->SetVisibility(VFXSettings.MuzzleScale > 0.f);
        GetWorldTimerManager().SetTimer(MuzzleLightTimer, this, &ATFLaserVFX::DisableMuzzleLight, VFXSettings.LightLifetime, false);
        ImpactLight->SetRelativeLocation(BoltEnd - BoltStart);
        ImpactLight->SetLightColor(VFXSettings.ImpactColor);
        ImpactLight->SetIntensity(VFXSettings.LightIntensity * VFXSettings.ImpactScale);
        ImpactLight->SetAttenuationRadius(VFXSettings.LightRadius);
        ImpactLight->SetCastShadows(VFXSettings.bCastImpactShadows);
        ImpactLight->SetVolumetricScatteringIntensity(VFXSettings.VolumetricScatteringIntensity);
        ImpactLight->SetVisibility(false);
    }
}

bool ATFLaserVFX::SpawnNiagaraSystem(UNiagaraSystem* System, const FVector& Location, const FRotator& Rotation, float Intensity, bool bConfigureBolt)
{
    if (!System) return false;
    UNiagaraComponent* Component = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, System, Location, Rotation, FVector::OneVector, true, true, ENCPoolMethod::AutoRelease, true);
    if (!Component) return false;
    Component->SetVariableLinearColor(TEXT("User.BoltColor"), VFXSettings.BoltColor);
    Component->SetVariableLinearColor(TEXT("User.CoreColor"), VFXSettings.CoreColor);
    Component->SetVariableLinearColor(TEXT("User.GlowColor"), VFXSettings.GlowColor);
    Component->SetVariableLinearColor(TEXT("User.MuzzleColor"), VFXSettings.MuzzleColor);
    Component->SetVariableLinearColor(TEXT("User.ImpactColor"), VFXSettings.ImpactColor);
    Component->SetVariableFloat(TEXT("User.Intensity"), Intensity);
    if (bConfigureBolt)
    {
        Component->SetVariableVec3(TEXT("User.BoltStart"), BoltStart);
        Component->SetVariableVec3(TEXT("User.BoltDirection"), BoltDirection);
        Component->SetVariableFloat(TEXT("User.BoltLength"), VFXSettings.BoltLength);
        Component->SetVariableFloat(TEXT("User.BoltWidth"), VFXSettings.BoltWidth);
        Component->SetVariableFloat(TEXT("User.GlowWidth"), VFXSettings.GlowWidth);
        Component->SetVariableFloat(TEXT("User.BoltSpeed"), VFXSettings.BoltSpeed);
        Component->SetVariableFloat(TEXT("User.BoltLifetime"), BoltTravelDuration);
        Component->SetVariableFloat(TEXT("User.TrailLength"), VFXSettings.TrailLength);
        Component->SetVariableFloat(TEXT("User.TrailWidth"), VFXSettings.TrailWidth);
    }
    return true;
}

void ATFLaserVFX::DrawFallbackBolt() const
{
    if (!GetWorld()) return;
    const float Elapsed = GetWorld()->GetTimeSeconds() - BoltSpawnTime;
    const float Progress = FMath::Clamp(Elapsed / BoltTravelDuration, 0.f, 1.f);
    const FVector Head = FMath::Lerp(BoltStart, BoltEnd, Progress);
    const float TravelledDistance = FVector::Distance(BoltStart, Head);
    const FVector Tail = Head - BoltDirection * FMath::Min(VFXSettings.BoltLength, TravelledDistance);
    FVector JitterOffset = FVector::ZeroVector;
    if (VFXSettings.BoltJitter > 0.f)
    {
        const FVector Side = FVector::CrossProduct(BoltDirection, FVector::UpVector).GetSafeNormal();
        JitterOffset = Side * FMath::Sin(Elapsed * 180.f) * VFXSettings.BoltJitter;
    }
    const float Duration = FMath::Min(.006f, FMath::Max(.002f, BoltTravelDuration / 8.f));
    DrawDebugLine(GetWorld(), Tail + JitterOffset, Head + JitterOffset, VFXSettings.GlowColor.ToFColor(true), false, Duration, 0, VFXSettings.GlowWidth);
    DrawDebugLine(GetWorld(), Tail + JitterOffset, Head + JitterOffset, VFXSettings.CoreColor.ToFColor(true), false, Duration, 0, VFXSettings.BoltWidth);
    if (VFXSettings.TrailLength > 0.f && VFXSettings.TrailWidth > 0.f)
    {
        const FVector TrailStart = Tail - BoltDirection * VFXSettings.TrailLength;
        DrawDebugLine(GetWorld(), TrailStart + JitterOffset, Tail + JitterOffset, VFXSettings.BoltColor.ToFColor(true), false, Duration, 0, VFXSettings.TrailWidth);
    }
}

void ATFLaserVFX::DrawFallbackMuzzle() const
{
    if (!GetWorld()) return;
    DrawDebugPoint(GetWorld(), BoltStart, FMath::Max(5.f, VFXSettings.MuzzleScale * 16.f), VFXSettings.MuzzleColor.ToFColor(true), false, VFXSettings.MuzzleLifetime);
    DrawDebugPoint(GetWorld(), BoltStart, FMath::Max(3.f, VFXSettings.MuzzleScale * 9.f), VFXSettings.CoreColor.ToFColor(true), false, VFXSettings.MuzzleLifetime);
}

void ATFLaserVFX::DrawFallbackImpact() const
{
    if (!GetWorld()) return;
    DrawDebugSphere(GetWorld(), BoltEnd, FMath::Max(10.f, VFXSettings.ImpactScale * 18.f), 8, VFXSettings.ImpactColor.ToFColor(true), false, VFXSettings.ImpactLifetime, 0, 1.5f);
    DrawDebugPoint(GetWorld(), BoltEnd, FMath::Max(7.f, VFXSettings.ImpactScale * 20.f), VFXSettings.CoreColor.ToFColor(true), false, VFXSettings.ImpactLifetime);
    const FVector Side = FVector::CrossProduct(BoltDirection, FVector::UpVector).GetSafeNormal();
    const FVector Up = FVector::CrossProduct(Side, BoltDirection).GetSafeNormal();
    for (int32 SparkIndex = 0; SparkIndex < 4; ++SparkIndex)
    {
        const float SideSign = SparkIndex % 2 == 0 ? 1.f : -1.f;
        const float UpSign = SparkIndex < 2 ? 1.f : -1.f;
        const FVector SparkDirection = (BoltDirection * .25f + Side * SideSign + Up * UpSign * .55f).GetSafeNormal();
        DrawDebugLine(GetWorld(), BoltEnd, BoltEnd + SparkDirection * (12.f + SparkIndex * 4.f), VFXSettings.CoreColor.ToFColor(true), false, VFXSettings.ImpactLifetime, 0, 1.4f);
    }
}

void ATFLaserVFX::AdvanceFallbackBolt()
{
    if (!GetWorld() || GetWorld()->GetTimeSeconds() - BoltSpawnTime >= BoltTravelDuration)
    {
        GetWorldTimerManager().ClearTimer(BoltStepTimer);
        return;
    }
    DrawFallbackBolt();
}

void ATFLaserVFX::PlayImpact()
{
    const FRotator BoltRotation = BoltDirection.Rotation();
    if (!SpawnNiagaraSystem(VFXSettings.ImpactSystem, BoltEnd, BoltRotation, VFXSettings.ImpactScale, false)) DrawFallbackImpact();
    if (VFXSettings.VFXQuality == ETFLaserVFXQuality::High) ImpactLight->SetVisibility(VFXSettings.ImpactScale > 0.f);
}

void ATFLaserVFX::DisableMuzzleLight()
{
    MuzzleLight->SetVisibility(false);
}
