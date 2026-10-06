#include "TFFormationWidget.h"
#include "TFPlayerController.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SUniformGridPanel.h"

TSharedRef<SWidget> UTFFormationWidget::RebuildWidget()
{
    const TWeakObjectPtr<UTFFormationWidget> WeakWidget(this);
    return SNew(SBorder)
        .Padding(FMargin(8.f))
        .BorderBackgroundColor(FLinearColor(.02f, .025f, .035f, .9f))
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 5.f)
            [ SNew(STextBlock).Text_Lambda([WeakWidget]() { return WeakWidget.IsValid() ? WeakWidget->GetStatusText() : FText::GetEmpty(); }) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
            [ SNew(SButton).OnClicked_UObject(this, &UTFFormationWidget::SelectLine)[SNew(STextBlock).Text(FText::FromString(TEXT("Linie")))] ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
            [ SNew(SButton).OnClicked_UObject(this, &UTFFormationWidget::SelectColumn)[SNew(STextBlock).Text(FText::FromString(TEXT("Kolonne")))] ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
            [ SNew(SButton).OnClicked_UObject(this, &UTFFormationWidget::SelectWedge)[SNew(STextBlock).Text(FText::FromString(TEXT("Keil")))] ]
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 2.f)
            [ SNew(STextBlock).Text_Lambda([WeakWidget]() { return WeakWidget.IsValid() && WeakWidget->Controller ? FText::FromString(FString::Printf(TEXT("Reihen: %s"), *WeakWidget->Controller->GetFormationRowsName())) : FText::GetEmpty(); }) ]
            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(SUniformGridPanel).SlotPadding(FMargin(1.f))
                + SUniformGridPanel::Slot(0, 0)[SNew(SButton).OnClicked_Lambda([WeakWidget]() { return WeakWidget.IsValid() ? WeakWidget->SelectRows(0) : FReply::Unhandled(); })[SNew(STextBlock).Text(FText::FromString(TEXT("Auto")))]]
                + SUniformGridPanel::Slot(1, 0)[SNew(SButton).OnClicked_Lambda([WeakWidget]() { return WeakWidget.IsValid() ? WeakWidget->SelectRows(1) : FReply::Unhandled(); })[SNew(STextBlock).Text(FText::FromString(TEXT("1")))]]
                + SUniformGridPanel::Slot(2, 0)[SNew(SButton).OnClicked_Lambda([WeakWidget]() { return WeakWidget.IsValid() ? WeakWidget->SelectRows(2) : FReply::Unhandled(); })[SNew(STextBlock).Text(FText::FromString(TEXT("2")))]]
                + SUniformGridPanel::Slot(0, 1)[SNew(SButton).OnClicked_Lambda([WeakWidget]() { return WeakWidget.IsValid() ? WeakWidget->SelectRows(3) : FReply::Unhandled(); })[SNew(STextBlock).Text(FText::FromString(TEXT("3")))]]
                + SUniformGridPanel::Slot(1, 1)[SNew(SButton).OnClicked_Lambda([WeakWidget]() { return WeakWidget.IsValid() ? WeakWidget->SelectRows(4) : FReply::Unhandled(); })[SNew(STextBlock).Text(FText::FromString(TEXT("4")))]]
                + SUniformGridPanel::Slot(2, 1)[SNew(SButton).OnClicked_Lambda([WeakWidget]() { return WeakWidget.IsValid() ? WeakWidget->SelectRows(5) : FReply::Unhandled(); })[SNew(STextBlock).Text(FText::FromString(TEXT("5")))]]
            ]
        ];
}
void UTFFormationWidget::SetController(ATFPlayerController* InController) { Controller = InController; }
FText UTFFormationWidget::GetStatusText() const
{
    return Controller ? FText::FromString(FString::Printf(TEXT("%d ausgewählt | Formation: %s"), Controller->SelectedUnits.Num(), *Controller->GetFormationName())) : FText::GetEmpty();
}
FReply UTFFormationWidget::SelectLine() { if (Controller) Controller->SetFormation(ETFFormation::Line); return FReply::Handled(); }
FReply UTFFormationWidget::SelectColumn() { if (Controller) Controller->SetFormation(ETFFormation::Column); return FReply::Handled(); }
FReply UTFFormationWidget::SelectWedge() { if (Controller) Controller->SetFormation(ETFFormation::Wedge); return FReply::Handled(); }
FReply UTFFormationWidget::SelectRows(int32 NewRows) { if (Controller) Controller->SetFormationRows(NewRows); return FReply::Handled(); }
