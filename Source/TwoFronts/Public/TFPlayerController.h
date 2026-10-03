#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TFTypes.h"
#include "TFPlayerController.generated.h"
class ATFUnit;
class ATFFactory;

UCLASS()
class TWOFRONTS_API ATFPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    ATFPlayerController();
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    UPROPERTY(BlueprintReadOnly) TArray<TObjectPtr<ATFUnit>> SelectedUnits;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<ATFFactory> SelectedFactory;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) ETFactionId RequestedFaction = ETFactionId::Humans;
    UFUNCTION(Server, Reliable) void ServerSetFaction(ETFactionId NewFaction);
    UFUNCTION(Server, Reliable) void ServerMoveUnits(const TArray<ATFUnit*>& Units, FVector Destination);
    UFUNCTION(Server, Reliable) void ServerAttackUnits(const TArray<ATFUnit*>& Units, ATFUnit* Target);
    UFUNCTION(Server, Reliable) void ServerRepairUnits(const TArray<ATFUnit*>& Units, ATFUnit* Target);
    UFUNCTION(Server, Reliable) void ServerEnqueueFactory(ATFFactory* Factory, int32 Option);
    ETFactionId GetPlayerFaction() const;
private:
    FVector2D SelectionStart;
    bool bSelectionInProgress = false;
    void BeginSelection(); void EndSelection(); void IssueCommand();
    void ChooseHumans(); void ChooseSynth();
    void SelectActor(AActor* Actor, bool bAppend); void ClearSelection();
    bool GetWorldHit(FHitResult& OutHit) const;
};
