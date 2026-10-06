#include "TFPlayerController.h"
#include "TFUnit.h"
#include "TFUnitDefinition.h"
#include "TFFactory.h"
#include "TFPlayerState.h"
#include "AIController.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "Components/CapsuleComponent.h"
#include "Blueprint/UserWidget.h"
#include "TFFormationWidget.h"
#include "TFFormationPlanner.h"

DEFINE_LOG_CATEGORY_STATIC(LogTwoFrontsRTS, Log, All);

ATFPlayerController::ATFPlayerController() { bShowMouseCursor = true; bEnableClickEvents = true; bEnableMouseOverEvents = true; }
void ATFPlayerController::BeginPlay()
{
    Super::BeginPlay();
    ServerSetFaction(RequestedFaction);
    if (IsLocalController())
    {
        FormationWidget = CreateWidget<UTFFormationWidget>(this, UTFFormationWidget::StaticClass());
        if (FormationWidget)
        {
            FormationWidget->SetController(this);
            FormationWidget->AddToViewport();
            FormationWidget->SetPositionInViewport(FVector2D(16.f, 135.f), true);
            FormationWidget->SetDesiredSizeInViewport(FVector2D(250.f, 250.f));
        }
        FInputModeGameAndUI InputMode;
        InputMode.SetHideCursorDuringCapture(false);
        SetInputMode(InputMode);
    }
}
void ATFPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindAction(TEXT("RTS_Select"), IE_Pressed, this, &ATFPlayerController::BeginSelection);
    InputComponent->BindAction(TEXT("RTS_Select"), IE_Released, this, &ATFPlayerController::EndSelection);
    InputComponent->BindAction(TEXT("RTS_Command"), IE_Pressed, this, &ATFPlayerController::BeginCommand);
    InputComponent->BindAction(TEXT("RTS_Command"), IE_Released, this, &ATFPlayerController::EndCommand);
    InputComponent->BindAction(TEXT("RTS_Humans"), IE_Pressed, this, &ATFPlayerController::ChooseHumans);
    InputComponent->BindAction(TEXT("RTS_Synth"), IE_Pressed, this, &ATFPlayerController::ChooseSynth);
    InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ATFPlayerController::CancelCommand);
}
void ATFPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    if (!bCommandHeld || SelectedUnits.IsEmpty()) return;

    FVector2D CurrentMouse;
    GetMousePosition(CurrentMouse.X, CurrentMouse.Y);
    FHitResult CurrentHit;
    if (!GetWorldHit(CurrentHit)) return;

    CommandCurrentLocation = CurrentHit.Location;
    if (FVector2D::Distance(CommandStartScreen, CurrentMouse) >= 18.f)
    {
        bLineCommandActive = true;
        PreviewTargets = BuildLineFormationTargets(CommandStartLocation, CommandCurrentLocation);
    }
}
ETFactionId ATFPlayerController::GetPlayerFaction() const
{
    if (!HasAuthority()) return RequestedFaction;
    return GetPlayerState<ATFPlayerState>() ? GetPlayerState<ATFPlayerState>()->ChosenFaction : RequestedFaction;
}
void ATFPlayerController::ChooseHumans() { RequestedFaction = ETFactionId::Humans; ServerSetFaction(RequestedFaction); ClearSelection(); }
void ATFPlayerController::ChooseSynth() { RequestedFaction = ETFactionId::Synth; ServerSetFaction(RequestedFaction); ClearSelection(); }
void ATFPlayerController::ServerSetFaction_Implementation(ETFactionId NewFaction) { if (ATFPlayerState* State = GetPlayerState<ATFPlayerState>()) State->ChosenFaction = (NewFaction == ETFactionId::Synth ? ETFactionId::Synth : ETFactionId::Humans); }
bool ATFPlayerController::GetWorldHit(FHitResult& OutHit) const { return GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Visibility), false, OutHit); }
void ATFPlayerController::BeginSelection() { GetMousePosition(SelectionStart.X, SelectionStart.Y); bSelectionInProgress = true; }
void ATFPlayerController::ClearSelection()
{
    for (ATFUnit* Unit : SelectedUnits) if (Unit) Unit->SetSelectedVisual(false);
    SelectedUnits.Empty(); SelectedFactory = nullptr;
}
void ATFPlayerController::SelectActor(AActor* Actor, bool bAppend)
{
    if (!bAppend) ClearSelection();
    if (ATFUnit* Unit = Cast<ATFUnit>(Actor))
    {
        if (Unit->CanReceiveOrdersFrom(GetPlayerFaction()))
        {
            if (bAppend && SelectedUnits.Contains(Unit))
            {
                SelectedUnits.Remove(Unit);
                Unit->SetSelectedVisual(false);
            }
            else
            {
                SelectedUnits.AddUnique(Unit);
                Unit->SetSelectedVisual(true);
            }
        }
    }
    if (ATFFactory* Factory = Cast<ATFFactory>(Actor)) if (Factory->CanReceiveOrdersFrom(GetPlayerFaction())) SelectedFactory = Factory;
}
void ATFPlayerController::EndSelection()
{
    if (!bSelectionInProgress) return; bSelectionInProgress = false;
    FVector2D End; GetMousePosition(End.X, End.Y);
    int32 SX = 0, SY = 0; GetViewportSize(SX, SY);
    // Production UI occupies the lower-right 720x170 pixels.
    if (SelectedFactory && End.X >= SX - 720 && End.Y >= SY - 200)
    {
        const int32 Slot = FMath::Clamp(FMath::FloorToInt((End.X - (SX - 720)) / 180.f), 0, 3);
        ServerEnqueueFactory(SelectedFactory, Slot); return;
    }
    const bool bAppend = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);
    if (FVector2D::Distance(SelectionStart, End) < 8.f)
    {
        FHitResult Hit; if (GetWorldHit(Hit)) SelectActor(Hit.GetActor(), bAppend); else if (!bAppend) ClearSelection();
        return;
    }
    if (!bAppend) ClearSelection();
    const float MinX = FMath::Min(SelectionStart.X, End.X), MaxX = FMath::Max(SelectionStart.X, End.X), MinY = FMath::Min(SelectionStart.Y, End.Y), MaxY = FMath::Max(SelectionStart.Y, End.Y);
    for (TActorIterator<ATFUnit> It(GetWorld()); It; ++It)
    {
        FVector2D Screen; if (ProjectWorldLocationToScreen(It->GetActorLocation(), Screen) && Screen.X >= MinX && Screen.X <= MaxX && Screen.Y >= MinY && Screen.Y <= MaxY && It->CanReceiveOrdersFrom(GetPlayerFaction())) { SelectedUnits.AddUnique(*It); It->SetSelectedVisual(true); }
    }
}
void ATFPlayerController::BeginCommand()
{
    if (SelectedUnits.IsEmpty()) return;
    if (!GetWorldHit(CommandStartHit))
    {
        UE_LOG(LogTwoFrontsRTS, Warning, TEXT("RTS move command rejected because the cursor did not hit a blocking world surface."));
        return;
    }
    GetMousePosition(CommandStartScreen.X, CommandStartScreen.Y);
    CommandStartLocation = CommandStartHit.Location;
    CommandCurrentLocation = CommandStartLocation;
    bCommandHeld = true;
    bLineCommandActive = false;
    PreviewTargets.Empty();
}
void ATFPlayerController::EndCommand()
{
    if (!bCommandHeld) return;
    bCommandHeld = false;
    if (bLineCommandActive)
    {
        FVector LineDirection = (CommandCurrentLocation - CommandStartLocation).GetSafeNormal2D();
        FVector DesiredFacing(-LineDirection.Y, LineDirection.X, 0.f);
        if (FVector::DotProduct(DesiredFacing, GetSelectionCenter() - (CommandStartLocation + CommandCurrentLocation) * .5f) < 0.f) DesiredFacing *= -1.f;
        IssueMoveCommand(PreviewTargets, DesiredFacing);
    }
    else IssueShortCommand(CommandStartHit);
    bLineCommandActive = false;
    PreviewTargets.Empty();
}
void ATFPlayerController::CancelCommand()
{
    bCommandHeld = false;
    bLineCommandActive = false;
    PreviewTargets.Empty();
}
void ATFPlayerController::IssueShortCommand(const FHitResult& Hit)
{
    if (ATFUnit* HitUnit = Cast<ATFUnit>(Hit.GetActor()))
    {
        if (HitUnit->Faction != GetPlayerFaction()) ServerAttackUnits(SelectedUnits, HitUnit);
        else ServerRepairUnits(SelectedUnits, HitUnit);
    }
    else IssueMoveCommand(BuildStandardFormationTargets(Hit.Location), Hit.Location - GetSelectionCenter());
}
void ATFPlayerController::IssueMoveCommand(const TArray<FVector>& Destinations, FVector DesiredFacing)
{
    if (SelectedUnits.IsEmpty() || Destinations.Num() != SelectedUnits.Num()) return;
    TArray<ATFUnit*> Units;
    Units.Reserve(SelectedUnits.Num());
    for (ATFUnit* Unit : SelectedUnits) Units.Add(Unit);
    ServerMoveUnits(Units, Destinations, DesiredFacing);
}
void ATFPlayerController::ServerMoveUnits_Implementation(const TArray<ATFUnit*>& Units, const TArray<FVector>& Destinations, FVector DesiredFacing)
{
    if (Units.IsEmpty() || Units.Num() != Destinations.Num())
    {
        UE_LOG(LogTwoFrontsRTS, Warning, TEXT("RTS move command rejected because its unit and destination counts differ."));
        return;
    }
    UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld());
    if (!NavigationSystem)
    {
        UE_LOG(LogTwoFrontsRTS, Warning, TEXT("RTS move command rejected because no NavigationSystem exists."));
        return;
    }

    const ETFactionId PlayerFaction = GetPlayerFaction();
    DesiredFacing.Z = 0.f;
    DesiredFacing = DesiredFacing.GetSafeNormal();
    int32 IssuedMoveCount = 0;
    TArray<FVector> AcceptedLocations;
    TArray<float> AcceptedRadii;
    for (int32 Index = 0; Index < Units.Num(); ++Index)
    {
        ATFUnit* Unit = Units[Index];
        if (!IsValid(Unit) || !Unit->CanReceiveOrdersFrom(PlayerFaction)) continue;

        FNavLocation NavigableDestination;
        if (!NavigationSystem->ProjectPointToNavigation(Destinations[Index], NavigableDestination, FVector(250.f, 250.f, 500.f)))
        {
            UE_LOG(LogTwoFrontsRTS, Warning, TEXT("RTS formation destination rejected for %s at %s."), *Unit->GetName(), *Destinations[Index].ToCompactString());
            continue;
        }
        const float UnitRadius = Unit->GetCapsuleComponent() ? Unit->GetCapsuleComponent()->GetScaledCapsuleRadius() : 34.f;
        bool bOverlapsAcceptedTarget = false;
        for (int32 AcceptedIndex = 0; AcceptedIndex < AcceptedLocations.Num(); ++AcceptedIndex)
        {
            if (FVector::DistSquared2D(NavigableDestination.Location, AcceptedLocations[AcceptedIndex]) < FMath::Square(UnitRadius + AcceptedRadii[AcceptedIndex] + 20.f))
            {
                bOverlapsAcceptedTarget = true;
                break;
            }
        }
        if (bOverlapsAcceptedTarget)
        {
            UE_LOG(LogTwoFrontsRTS, Warning, TEXT("RTS formation destination rejected for %s because navigation projection would overlap another accepted target."), *Unit->GetName());
            continue;
        }

        Unit->ClearCombatTarget();
        Unit->SetRepairTarget(nullptr);
        Unit->SetFormationFacing(DesiredFacing.IsNearlyZero() ? NavigableDestination.Location - Unit->GetActorLocation() : DesiredFacing);
        if (!Unit->GetController()) Unit->SpawnDefaultController();

        AAIController* AIController = Cast<AAIController>(Unit->GetController());
        if (!AIController)
        {
            UE_LOG(LogTwoFrontsRTS, Warning, TEXT("RTS move command could not create an AI controller for %s."), *Unit->GetName());
            continue;
        }

        const EPathFollowingRequestResult::Type MoveResult = AIController->MoveToLocation(NavigableDestination.Location, 60.f);
        if (MoveResult == EPathFollowingRequestResult::Failed)
        {
            UE_LOG(LogTwoFrontsRTS, Warning, TEXT("RTS move command failed path submission for %s."), *Unit->GetName());
            continue;
        }

        ++IssuedMoveCount;
        AcceptedLocations.Add(NavigableDestination.Location);
        AcceptedRadii.Add(UnitRadius);
    }

    if (IssuedMoveCount == 0)
    {
        UE_LOG(LogTwoFrontsRTS, Warning, TEXT("RTS move command did not apply to an owned, living unit."));
        return;
    }

    UE_LOG(LogTwoFrontsRTS, Log, TEXT("RTS formation move command accepted for %d unit(s)."), IssuedMoveCount);
}
FString ATFPlayerController::GetFormationName() const
{
    switch (CurrentFormation)
    {
    case ETFFormation::Column: return TEXT("Kolonne");
    case ETFFormation::Wedge: return TEXT("Keil");
    default: return TEXT("Linie");
    }
}
void ATFPlayerController::SetFormation(ETFFormation NewFormation) { CurrentFormation = NewFormation; }
FString ATFPlayerController::GetFormationRowsName() const
{
    return CurrentFormationRows == 0 ? TEXT("Auto") : FString::FromInt(CurrentFormationRows);
}
void ATFPlayerController::SetFormationRows(int32 NewRows)
{
    CurrentFormationRows = FMath::Clamp(NewRows, 0, 5);
}
FVector ATFPlayerController::GetSelectionCenter() const
{
    FVector Center = FVector::ZeroVector;
    int32 Count = 0;
    for (const ATFUnit* Unit : SelectedUnits) if (IsValid(Unit)) { Center += Unit->GetActorLocation(); ++Count; }
    return Count > 0 ? Center / Count : FVector::ZeroVector;
}
FTFFormationPlan ATFPlayerController::BuildFormationPlan(const FVector& Anchor, const FVector& Forward, const FVector& Right, float FrontWidth) const
{
    TArray<FTFFormationUnit> Units;
    Units.Reserve(SelectedUnits.Num());
    for (const ATFUnit* Unit : SelectedUnits)
    {
        FTFFormationUnit& PlannerUnit = Units.AddDefaulted_GetRef();
        if (!IsValid(Unit)) continue;
        PlannerUnit.SourceLocation = Unit->GetActorLocation();
        PlannerUnit.WeaponRange = Unit->Definition ? Unit->Definition->Combat.Range : 0.f;
        PlannerUnit.CollisionRadius = Unit->GetCapsuleComponent() ? Unit->GetCapsuleComponent()->GetScaledCapsuleRadius() : 34.f;
    }
    FTFFormationPlanRequest Request;
    Request.Formation = CurrentFormation;
    Request.RequestedRows = CurrentFormationRows;
    Request.Anchor = Anchor;
    Request.Forward = Forward;
    Request.Right = Right;
    Request.FrontWidth = FrontWidth;
    return FTFFormationPlanner::BuildPlan(Units, Request);
}
TArray<FVector> ATFPlayerController::BuildStandardFormationTargets(const FVector& Destination) const
{
    if (SelectedUnits.IsEmpty()) return {};
    FVector Forward = (Destination - GetSelectionCenter()).GetSafeNormal2D();
    if (Forward.IsNearlyZero()) Forward = FVector::ForwardVector;
    const FVector Right(-Forward.Y, Forward.X, 0.f);
    return BuildFormationPlan(Destination, Forward, Right).Targets;
}
TArray<FVector> ATFPlayerController::BuildLineFormationTargets(FVector LineStart, FVector LineEnd) const
{
    if (SelectedUnits.IsEmpty()) return {};
    FVector LineDirection = (LineEnd - LineStart).GetSafeNormal2D();
    if (LineDirection.IsNearlyZero()) LineDirection = FVector::RightVector;
    FVector Forward(-LineDirection.Y, LineDirection.X, 0.f);
    if (FVector::DotProduct(Forward, GetSelectionCenter() - (LineStart + LineEnd) * .5f) < 0.f) Forward *= -1.f;
    return BuildFormationPlan((LineStart + LineEnd) * .5f, Forward, LineDirection, FVector::Dist2D(LineStart, LineEnd)).Targets;
}
void ATFPlayerController::ServerAttackUnits_Implementation(const TArray<ATFUnit*>& Units, ATFUnit* Target)
{
    if (!IsValid(Target) || Target->Faction == GetPlayerFaction()) return;
    for (ATFUnit* Unit : Units) if (IsValid(Unit) && Unit->CanReceiveOrdersFrom(GetPlayerFaction())) Unit->SetAttackTarget(Target);
}
void ATFPlayerController::ServerRepairUnits_Implementation(const TArray<ATFUnit*>& Units, ATFUnit* Target)
{
    if (!IsValid(Target) || Target->Faction != GetPlayerFaction()) return;
    for (ATFUnit* Unit : Units) if (IsValid(Unit) && Unit->CanReceiveOrdersFrom(GetPlayerFaction())) Unit->SetRepairTarget(Target);
}
void ATFPlayerController::ServerEnqueueFactory_Implementation(ATFFactory* Factory, int32 Option)
{
    if (IsValid(Factory) && Factory->CanReceiveOrdersFrom(GetPlayerFaction())) Factory->EnqueueProduction(Option);
}
