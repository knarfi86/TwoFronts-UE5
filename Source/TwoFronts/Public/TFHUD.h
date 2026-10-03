#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "TFHUD.generated.h"
UCLASS()
class TWOFRONTS_API ATFHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
};
