#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TFFormationWidget.generated.h"

class ATFPlayerController;
class SWidget;

UCLASS()
class TWOFRONTS_API UTFFormationWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    void SetController(ATFPlayerController* InController);
private:
    UPROPERTY() TObjectPtr<ATFPlayerController> Controller;
    FReply SelectLine();
    FReply SelectColumn();
    FReply SelectWedge();
    FReply SelectRows(int32 NewRows);
    FText GetStatusText() const;
};
