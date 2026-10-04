#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TFTypes.h"
#include "TFPlayerController.generated.h"
class ATFUnit;
class ATFFactory;
class UTFFormationWidget;
class UTFUnitDefinition;
struct FTFFormationPlan;

UCLASS()
class TWOFRONTS_API ATFPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    ATFPlayerController();
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void PlayerTick(float DeltaTime) override;
    UPROPERTY(BlueprintReadOnly) TArray<TObjectPtr<ATFUnit>> SelectedUnits;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<ATFFactory> SelectedFactory;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) ETFactionId RequestedFaction = ETFactionId::Humans;
    UFUNCTION(Server, Reliable) void ServerSetFaction(ETFactionId NewFaction);
    UFUNCTION(Server, Reliable) void ServerMoveUnits(const TArray<ATFUnit*>& Units, const TArray<FVector>& Destinations, FVector DesiredFacing);
    UFUNCTION(Server, Reliable) void ServerAttackUnits(const TArray<ATFUnit*>& Units, ATFUnit* Target);
    UFUNCTION(Server, Reliable) void ServerRepairUnits(const TArray<ATFUnit*>& Units, ATFUnit* Target);
    UFUNCTION(Server, Reliable) void ServerEnqueueFactory(ATFFactory* Factory, int32 Option);
    UFUNCTION(Server, Reliable) void ServerStartCombatDemo();
    ETFactionId GetPlayerFaction() const;
    ETFFormation GetFormation() const { return CurrentFormation; }
    FString GetFormationName() const;
    void SetFormation(ETFFormation NewFormation);
    int32 GetFormationRows() const { return CurrentFormationRows; }
    FString GetFormationRowsName() const;
    void SetFormationRows(int32 NewRows);
    FString GetCombatDemoStatus() const;
    int32 GetCombatDemoRemaining(ETFactionId Faction) const;
    bool IsSelectionInProgress() const { return bSelectionInProgress; }
    FVector2D GetSelectionStart() const { return SelectionStart; }
    bool IsLinePreviewActive() const { return bLineCommandActive; }
    const TArray<FVector>& GetPreviewTargets() const { return PreviewTargets; }
    FVector GetPreviewLineStart() const { return CommandStartLocation; }
    FVector GetPreviewLineEnd() const { return CommandCurrentLocation; }
private:
    FVector2D SelectionStart;
    bool bSelectionInProgress = false;
    bool bCommandHeld = false;
    bool bLineCommandActive = false;
    FVector2D CommandStartScreen;
    FVector CommandStartLocation = FVector::ZeroVector;
    FVector CommandCurrentLocation = FVector::ZeroVector;
    FHitResult CommandStartHit;
    TArray<FVector> PreviewTargets;
    ETFFormation CurrentFormation = ETFFormation::Line;
    int32 CurrentFormationRows = 0;
    TObjectPtr<UTFUnitDefinition> LastClickedUnitDefinition;
    float LastUnitClickTime = -100.f;
    UPROPERTY() TObjectPtr<UTFFormationWidget> FormationWidget;
    void BeginSelection(); void EndSelection();
    void BeginCommand(); void EndCommand(); void CancelCommand();
    void ChooseHumans(); void ChooseSynth();
    void SelectActor(AActor* Actor, bool bAppend); void ClearSelection();
    void SelectUnitsOfDefinition(const UTFUnitDefinition* UnitDefinition);
    void IssueShortCommand(const FHitResult& Hit);
    void IssueMoveCommand(const TArray<FVector>& Destinations, FVector DesiredFacing);
    TArray<FVector> BuildStandardFormationTargets(const FVector& Destination) const;
    TArray<FVector> BuildLineFormationTargets(FVector LineStart, FVector LineEnd) const;
    FTFFormationPlan BuildFormationPlan(const FVector& Anchor, const FVector& Forward, const FVector& Right, float FrontWidth = 0.f) const;
    FVector GetSelectionCenter() const;
    bool GetWorldHit(FHitResult& OutHit) const;
};
