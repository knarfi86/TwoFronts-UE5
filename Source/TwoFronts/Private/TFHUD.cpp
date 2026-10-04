#include "TFHUD.h"
#include "TFPlayerController.h"
#include "TFUnit.h"
#include "TFFactory.h"
#include "TFUnitDefinition.h"
#include "TFHealthComponent.h"
#include "Engine/Canvas.h"

void ATFHUD::DrawHUD()
{
    Super::DrawHUD();
    ATFPlayerController* PC = Cast<ATFPlayerController>(PlayerOwner); if (!PC || !Canvas) return;
    const float W = Canvas->ClipX, H = Canvas->ClipY;
    const FLinearColor Panel(.02f, .025f, .035f, .85f), White = FLinearColor::White, Accent = PC->GetPlayerFaction() == ETFactionId::Humans ? FLinearColor(.95f, .38f, .08f) : FLinearColor(.05f, .78f, .95f);
    DrawRect(Panel, 16, 16, 420, 110);
    DrawText(FString::Printf(TEXT("Two Fronts V0.3.1  |  Spieler: %s"), PC->GetPlayerFaction() == ETFactionId::Humans ? TEXT("Humans") : TEXT("Synth")), Accent, 30, 28, nullptr, 1.1f);
    DrawText(FString::Printf(TEXT("[1] Humans  [2] Synth   LMB: wählen   RMB: %s   Reihen: %s"), PC->IsLinePreviewActive() ? TEXT("Linie ziehen") : TEXT("Formation"), *PC->GetFormationRowsName()), White, 30, 58);
    if (!PC->SelectedUnits.IsEmpty())
    {
        ATFUnit* Unit = PC->SelectedUnits[0];
        if (Unit && Unit->Definition) DrawText(FString::Printf(TEXT("%s  HP %.0f / %.0f  |  %d ausgewählt"), *Unit->Definition->DisplayName.ToString(), Unit->Health->CurrentHealth, Unit->Health->MaxHealth, PC->SelectedUnits.Num()), White, 30, 86);
    }
    DrawText(FString::Printf(TEXT("KAMPFTEST V0.3.1  Humans: %d/10  Synth: %d/10  %s"), PC->GetCombatDemoRemaining(ETFactionId::Humans), PC->GetCombatDemoRemaining(ETFactionId::Synth), *PC->GetCombatDemoStatus()), Accent, 30, 108);
    if (PC->IsSelectionInProgress())
    {
        FVector2D CurrentMouse; PC->GetMousePosition(CurrentMouse.X, CurrentMouse.Y);
        const FVector2D Start = PC->GetSelectionStart();
        const float Left = FMath::Min(Start.X, CurrentMouse.X), Top = FMath::Min(Start.Y, CurrentMouse.Y);
        const float Width = FMath::Abs(CurrentMouse.X - Start.X), Height = FMath::Abs(CurrentMouse.Y - Start.Y);
        DrawRect(FLinearColor(Accent.R, Accent.G, Accent.B, .12f), Left, Top, Width, Height);
        DrawLine(Left, Top, Left + Width, Top, Accent, 2.f); DrawLine(Left + Width, Top, Left + Width, Top + Height, Accent, 2.f);
        DrawLine(Left + Width, Top + Height, Left, Top + Height, Accent, 2.f); DrawLine(Left, Top + Height, Left, Top, Accent, 2.f);
    }
    if (PC->IsLinePreviewActive())
    {
        FVector2D LineStart, LineEnd;
        if (PC->ProjectWorldLocationToScreen(PC->GetPreviewLineStart(), LineStart) && PC->ProjectWorldLocationToScreen(PC->GetPreviewLineEnd(), LineEnd)) DrawLine(LineStart.X, LineStart.Y, LineEnd.X, LineEnd.Y, Accent, 3.f);
        const TArray<FVector>& Targets = PC->GetPreviewTargets();
        for (int32 Index = 0; Index < Targets.Num(); ++Index)
        {
            FVector2D ScreenTarget;
            const ATFUnit* Unit = PC->SelectedUnits.IsValidIndex(Index) ? PC->SelectedUnits[Index] : nullptr;
            const float Range = Unit && Unit->Definition ? Unit->Definition->Combat.Range : 0.f;
            const FLinearColor MarkerColor = Range <= 0.f ? FLinearColor(.75f, .75f, .75f) : (Range < 650.f ? FLinearColor(.95f, .35f, .12f) : (Range < 800.f ? FLinearColor(.95f, .8f, .12f) : FLinearColor(.2f, .75f, 1.f)));
            if (PC->ProjectWorldLocationToScreen(Targets[Index], ScreenTarget)) DrawRect(MarkerColor, ScreenTarget.X - 5.f, ScreenTarget.Y - 5.f, 10.f, 10.f);
        }
    }
    if (ATFFactory* Factory = PC->SelectedFactory)
    {
        DrawRect(Panel, W - 735, H - 220, 720, 205);
        DrawText(FString::Printf(TEXT("%s — Produktion (kostenlos)"), *Factory->DisplayName.ToString()), Accent, W - 720, H - 205, nullptr, 1.1f);
        for (int32 Index = 0; Index < Factory->ProductionOptions.Num() && Index < 4; ++Index)
        {
            const UTFUnitDefinition* Def = Factory->ProductionOptions[Index]; const float X = W - 720 + Index * 180.f;
            DrawRect(FLinearColor(.12f, .15f, .19f, 1.f), X, H - 165, 165, 75);
            DrawText(FString::Printf(TEXT("%d: %s"), Index + 1, *Def->DisplayName.ToString()), White, X + 8, H - 157);
            DrawText(FString::Printf(TEXT("%.1f Sek."), Def->ProductionSeconds), Accent, X + 8, H - 130);
        }
        FString Queue = TEXT("Warteschlange: ");
        for (const FTFProductionItem& Item : Factory->ProductionQueue) Queue += FString::Printf(TEXT("%s (%.1fs)  "), *Item.UnitDefinition->DisplayName.ToString(), Item.RemainingSeconds);
        DrawText(Queue, White, W - 720, H - 65);
    }
}
