#include "TFPlayerController.h"
#include "TFUnit.h"
#include "TFFactory.h"
#include "TFPlayerState.h"
#include "AIController.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogTwoFrontsRTS, Log, All);

ATFPlayerController::ATFPlayerController() { bShowMouseCursor = true; bEnableClickEvents = true; bEnableMouseOverEvents = true; }
void ATFPlayerController::BeginPlay() { Super::BeginPlay(); ServerSetFaction(RequestedFaction); }
void ATFPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindAction(TEXT("RTS_Select"), IE_Pressed, this, &ATFPlayerController::BeginSelection);
    InputComponent->BindAction(TEXT("RTS_Select"), IE_Released, this, &ATFPlayerController::EndSelection);
    InputComponent->BindAction(TEXT("RTS_Command"), IE_Pressed, this, &ATFPlayerController::IssueCommand);
    InputComponent->BindAction(TEXT("RTS_Humans"), IE_Pressed, this, &ATFPlayerController::ChooseHumans);
    InputComponent->BindAction(TEXT("RTS_Synth"), IE_Pressed, this, &ATFPlayerController::ChooseSynth);
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
    if (ATFUnit* Unit = Cast<ATFUnit>(Actor)) { if (Unit->CanReceiveOrdersFrom(GetPlayerFaction())) { SelectedUnits.AddUnique(Unit); Unit->SetSelectedVisual(true); } }
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
void ATFPlayerController::IssueCommand()
{
    if (SelectedUnits.IsEmpty()) return;
    UE_LOG(LogTwoFrontsRTS, Log, TEXT("RTS command input received for %d selected unit(s)."), SelectedUnits.Num());
    FHitResult Hit;
    if (!GetWorldHit(Hit))
    {
        UE_LOG(LogTwoFrontsRTS, Warning, TEXT("RTS move command rejected because the cursor did not hit a blocking world surface."));
        return;
    }
    if (ATFUnit* HitUnit = Cast<ATFUnit>(Hit.GetActor()))
    {
        if (HitUnit->Faction != GetPlayerFaction()) ServerAttackUnits(SelectedUnits, HitUnit);
        else ServerRepairUnits(SelectedUnits, HitUnit);
    }
    else ServerMoveUnits(SelectedUnits, Hit.Location);
}
void ATFPlayerController::ServerMoveUnits_Implementation(const TArray<ATFUnit*>& Units, FVector Destination)
{
    UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld());
    FNavLocation NavigableDestination;
    if (!NavigationSystem || !NavigationSystem->ProjectPointToNavigation(Destination, NavigableDestination, FVector(250.f, 250.f, 500.f)))
    {
        UE_LOG(LogTwoFrontsRTS, Warning, TEXT("RTS move command rejected because no reachable NavMesh point exists at %s."), *Destination.ToCompactString());
        return;
    }

    const ETFactionId PlayerFaction = GetPlayerFaction();
    int32 IssuedMoveCount = 0;
    for (ATFUnit* Unit : Units)
    {
        if (!IsValid(Unit) || !Unit->CanReceiveOrdersFrom(PlayerFaction)) continue;

        Unit->ClearCombatTarget();
        Unit->SetRepairTarget(nullptr);
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
    }

    if (IssuedMoveCount == 0)
    {
        UE_LOG(LogTwoFrontsRTS, Warning, TEXT("RTS move command did not apply to an owned, living unit."));
        return;
    }

    UE_LOG(LogTwoFrontsRTS, Log, TEXT("RTS move command accepted for %d unit(s) at %s."), IssuedMoveCount, *NavigableDestination.Location.ToCompactString());
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
