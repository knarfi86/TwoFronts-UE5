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
    if (!World || Settings.VFXQuality == ETFLaserVFXQuality::Low && Settings.BeamLifetime <= 0.f) return;
    if (ATFLaserVFX* Effect = World->SpawnActor<ATFLaserVFX>(MuzzleLocation, FRotator::ZeroRotator)) Effect->Initialise(MuzzleLocation, ImpactLocation, Settings);
}

void ATFLaserVFX::Initialise(const FVector& MuzzleLocation, const FVector& ImpactLocation, const FTFDirectShotVFXSettings& Settings)
{
    VFXSettings = Settings;
    BeamStart = MuzzleLocation;
    BeamEnd = ImpactLocation;
    SetActorLocation(BeamStart);
    InitialLifeSpan = FMath::Max(.01f, FMath::Max(VFXSettings.BeamLifetime, VFXSettings.VFXQuality == ETFLaserVFXQuality::High ? VFXSettings.LightLifetime : 0.f));

    const FRotator BeamRotation = (BeamEnd - BeamStart).Rotation();
    if (!SpawnNiagaraSystem(VFXSettings.BeamSystem, BeamStart, BeamRotation, true)) DrawFallbackBeam();
    if (VFXSettings.VFXQuality != ETFLaserVFXQuality::Low)
    {
        const bool bHasMuzzleVFX = SpawnNiagaraSystem(VFXSettings.MuzzleSystem, BeamStart, BeamRotation, false);
        const bool bHasImpactVFX = SpawnNiagaraSystem(VFXSettings.ImpactSystem, BeamEnd, BeamRotation, false);
        if (!bHasMuzzleVFX) DrawFallbackMuzzle();
        if (!bHasImpactVFX) DrawFallbackImpact();
    }
    if (VFXSettings.VFXQuality == ETFLaserVFXQuality::High)
    {
        MuzzleLight->SetLightColor(VFXSettings.PrimaryColor);
        MuzzleLight->SetIntensity(VFXSettings.LightIntensity * VFXSettings.MuzzleIntensity);
        MuzzleLight->SetAttenuationRadius(VFXSettings.LightRadius * .65f);
        MuzzleLight->SetVolumetricScatteringIntensity(VFXSettings.VolumetricScatteringIntensity);
        MuzzleLight->SetVisibility(VFXSettings.MuzzleIntensity > 0.f);
        ImpactLight->SetRelativeLocation(BeamEnd - BeamStart);
        ImpactLight->SetLightColor(VFXSettings.PrimaryColor);
        ImpactLight->SetIntensity(VFXSettings.LightIntensity * VFXSettings.ImpactIntensity);
        ImpactLight->SetAttenuationRadius(VFXSettings.LightRadius);
        ImpactLight->SetCastShadows(VFXSettings.bCastImpactShadows);
        ImpactLight->SetVolumetricScatteringIntensity(VFXSettings.VolumetricScatteringIntensity);
        ImpactLight->SetVisibility(VFXSettings.ImpactIntensity > 0.f);
    }
    if (!VFXSettings.BeamSystem && VFXSettings.BeamLifetime > .03f)
    {
        GetWorldTimerManager().SetTimer(FallbackPulseTimer, this, &ATFLaserVFX::PulseFallbackBeam, VFXSettings.BeamLifetime * .45f, true, VFXSettings.BeamLifetime * .45f);
    }
}

bool ATFLaserVFX::SpawnNiagaraSystem(UNiagaraSystem* System, const FVector& Location, const FRotator& Rotation, bool bConfigureBeam)
{
    if (!System) return false;
    UNiagaraComponent* Component = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, System, Location, Rotation, FVector::OneVector, true, true, ENCPoolMethod::AutoRelease, true);
    if (!Component) return false;
    Component->SetVariableLinearColor(TEXT("User.PrimaryColor"), VFXSettings.PrimaryColor);
    Component->SetVariableLinearColor(TEXT("User.CoreColor"), VFXSettings.CoreColor);
    Component->SetVariableFloat(TEXT("User.Intensity"), bConfigureBeam ? 1.f : (Location.Equals(BeamStart) ? VFXSettings.MuzzleIntensity : VFXSettings.ImpactIntensity));
    if (bConfigureBeam)
    {
        Component->SetVariableVec3(TEXT("User.BeamStart"), BeamStart);
        Component->SetVariableVec3(TEXT("User.BeamEnd"), BeamEnd);
        Component->SetVariableFloat(TEXT("User.BeamWidth"), VFXSettings.BeamWidth);
        Component->SetVariableFloat(TEXT("User.GlowWidth"), VFXSettings.GlowWidth);
        Component->SetVariableFloat(TEXT("User.BeamLifetime"), VFXSettings.BeamLifetime);
    }
    return true;
}

void ATFLaserVFX::DrawFallbackBeam(float WidthScale) const
{
    if (!GetWorld()) return;
    const float Duration = FMath::Max(.01f, VFXSettings.BeamLifetime * .7f);
    DrawDebugLine(GetWorld(), BeamStart, BeamEnd, VFXSettings.PrimaryColor.ToFColor(true), false, Duration, 0, VFXSettings.GlowWidth * WidthScale);
    DrawDebugLine(GetWorld(), BeamStart, BeamEnd, VFXSettings.CoreColor.ToFColor(true), false, Duration, 0, VFXSettings.BeamWidth * WidthScale);
}

void ATFLaserVFX::DrawFallbackMuzzle() const
{
    if (!GetWorld()) return;
    const float Duration = FMath::Max(.01f, VFXSettings.BeamLifetime * .65f);
    DrawDebugPoint(GetWorld(), BeamStart, FMath::Max(4.f, VFXSettings.MuzzleIntensity * 12.f), VFXSettings.PrimaryColor.ToFColor(true), false, Duration);
}

void ATFLaserVFX::DrawFallbackImpact() const
{
    if (!GetWorld()) return;
    const float Duration = FMath::Max(.01f, VFXSettings.BeamLifetime * .65f);
    DrawDebugSphere(GetWorld(), BeamEnd, FMath::Max(8.f, VFXSettings.ImpactIntensity * 14.f), 8, VFXSettings.PrimaryColor.ToFColor(true), false, Duration, 0, 1.5f);
    DrawDebugPoint(GetWorld(), BeamEnd, FMath::Max(5.f, VFXSettings.ImpactIntensity * 16.f), VFXSettings.CoreColor.ToFColor(true), false, Duration);
}

void ATFLaserVFX::PulseFallbackBeam()
{
    if (++PulseCount > 1 || !GetWorld()) { GetWorldTimerManager().ClearTimer(FallbackPulseTimer); return; }
    DrawFallbackBeam(1.18f);
}
