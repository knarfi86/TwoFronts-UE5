#include "TFPlayerController.h"
#include "TFUnit.h"
#include "TFFactory.h"
#include "TFPlayerState.h"
#include "AIController.h"
#include "Engine/World.h"
#include "EngineUtils.h"

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
ETFactionId ATFPlayerController::GetPlayerFaction() const { return GetPlayerState<ATFPlayerState>() ? GetPlayerState<ATFPlayerState>()->ChosenFaction : RequestedFaction; }
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
    FHitResult Hit; if (!GetWorldHit(Hit)) return;
    if (ATFUnit* HitUnit = Cast<ATFUnit>(Hit.GetActor()))
    {
        if (HitUnit->Faction != GetPlayerFaction()) ServerAttackUnits(SelectedUnits, HitUnit);
        else ServerRepairUnits(SelectedUnits, HitUnit);
    }
    else ServerMoveUnits(SelectedUnits, Hit.Location);
}
void ATFPlayerController::ServerMoveUnits_Implementation(const TArray<ATFUnit*>& Units, FVector Destination)
{
    for (ATFUnit* Unit : Units) if (IsValid(Unit) && Unit->CanReceiveOrdersFrom(GetPlayerFaction())) { Unit->ClearCombatTarget(); Unit->SetRepairTarget(nullptr); if (AAIController* AI = Cast<AAIController>(Unit->GetController())) AI->MoveToLocation(Destination, 60.f); }
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
