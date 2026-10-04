#include "TFRTSCameraPawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/FloatingPawnMovement.h"

ATFRTSCameraPawn::ATFRTSCameraPawn()
{
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm")); SpringArm->SetupAttachment(GetRootComponent()); SpringArm->TargetArmLength = 7500.f; SpringArm->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f)); SpringArm->bDoCollisionTest = false;
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera")); Camera->SetupAttachment(SpringArm);
    Camera->PostProcessBlendWeight = 1.f;
    Camera->PostProcessSettings.bOverride_AutoExposureMethod = true;
    Camera->PostProcessSettings.AutoExposureMethod = AEM_Manual;
    Camera->PostProcessSettings.bOverride_AutoExposureBias = true;
    Camera->PostProcessSettings.AutoExposureBias = 0.f;
    Camera->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
    Camera->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = false;
    CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"))->MaxSpeed = 2200.f;
}
void ATFRTSCameraPawn::SetupPlayerInputComponent(UInputComponent* Input) { Super::SetupPlayerInputComponent(Input); Input->BindAxis(TEXT("RTS_MoveForward"), this, &ATFRTSCameraPawn::MoveForward); Input->BindAxis(TEXT("RTS_MoveRight"), this, &ATFRTSCameraPawn::MoveRight); Input->BindAxis(TEXT("RTS_Zoom"), this, &ATFRTSCameraPawn::Zoom); Input->BindAxis(TEXT("RTS_Rotate"), this, &ATFRTSCameraPawn::Rotate); }
void ATFRTSCameraPawn::MoveForward(float Value) { AddMovementInput(FVector(GetActorForwardVector().X, GetActorForwardVector().Y, 0).GetSafeNormal(), Value); }
void ATFRTSCameraPawn::MoveRight(float Value) { AddMovementInput(FVector(GetActorRightVector().X, GetActorRightVector().Y, 0).GetSafeNormal(), Value); }
void ATFRTSCameraPawn::Zoom(float Value) { SpringArm->TargetArmLength = FMath::Clamp(SpringArm->TargetArmLength - Value * 180.f, MinZoom, MaxZoom); }
void ATFRTSCameraPawn::Rotate(float Value) { AddControllerYawInput(Value * 1.2f); }
