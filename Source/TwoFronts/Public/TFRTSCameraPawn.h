#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "TFRTSCameraPawn.generated.h"
class USpringArmComponent;
class UCameraComponent;
UCLASS()
class TWOFRONTS_API ATFRTSCameraPawn : public APawn
{
    GENERATED_BODY()
public:
    ATFRTSCameraPawn();
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    void MoveForward(float Value); void MoveRight(float Value); void Zoom(float Value); void Rotate(float Value);
    void LookYaw(float Value); void LookPitch(float Value);
    void SetFreeLookActive(bool bEnabled) { bFreeLookActive = bEnabled; }
    bool IsFreeLookActive() const { return bFreeLookActive; }
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float MinZoom = 900.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float MaxZoom = 10000.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera") float FreeLookYawSensitivity = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera") float FreeLookPitchSensitivity = 1.f;
private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> SpringArm;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
    bool bFreeLookActive = false;
};
