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
    DrawText(FString::Printf(TEXT("Two Fronts V0.1  |  Spieler: %s"), PC->GetPlayerFaction() == ETFactionId::Humans ? TEXT("Humans") : TEXT("Synth")), Accent, 30, 28, nullptr, 1.1f);
    DrawText(TEXT("[1] Humans  [2] Synth   LMB: wählen   RMB: Befehl"), White, 30, 58);
    if (!PC->SelectedUnits.IsEmpty())
    {
        ATFUnit* Unit = PC->SelectedUnits[0];
        if (Unit && Unit->Definition) DrawText(FString::Printf(TEXT("%s  HP %.0f / %.0f  |  %d ausgewählt"), *Unit->Definition->DisplayName.ToString(), Unit->Health->CurrentHealth, Unit->Health->MaxHealth, PC->SelectedUnits.Num()), White, 30, 86);
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
