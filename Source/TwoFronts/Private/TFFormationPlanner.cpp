#include "TFFormationPlanner.h"

namespace
{
struct FFormationSlot
{
    FVector Location;
    int32 Row = 0;
};

int32 ResolveRows(const int32 UnitCount, const int32 RequestedRows, const int32 SlotsPerRow, const bool bDrawnFront)
{
    if (RequestedRows > 0) return FMath::Clamp(RequestedRows, 1, UnitCount);
    if (bDrawnFront) return FMath::Max(1, FMath::DivideAndRoundUp(UnitCount, FMath::Max(1, SlotsPerRow)));
    return FMath::Clamp(FMath::CeilToInt(FMath::Sqrt(static_cast<float>(UnitCount))), 1, 5);
}

void AddCenteredRow(TArray<FFormationSlot>& Slots, const FVector& RowCenter, const FVector& Right, const int32 Count, const float Spacing, const int32 Row)
{
    for (int32 Column = 0; Column < Count; ++Column)
    {
        Slots.Add({ RowCenter + Right * ((Column - (Count - 1) * .5f) * Spacing), Row });
    }
}
}

float FTFFormationPlanner::CalculateMinimumSpacing(const TArray<FTFFormationUnit>& Units)
{
    float LargestRadius = 0.f;
    for (const FTFFormationUnit& Unit : Units) LargestRadius = FMath::Max(LargestRadius, Unit.CollisionRadius);
    return FMath::Max(180.f, LargestRadius * 2.f + 80.f);
}

FTFFormationPlan FTFFormationPlanner::BuildPlan(const TArray<FTFFormationUnit>& Units, const FTFFormationPlanRequest& Request)
{
    FTFFormationPlan Plan;
    if (Units.IsEmpty()) return Plan;

    FVector Forward = Request.Forward.GetSafeNormal2D();
    if (Forward.IsNearlyZero()) Forward = FVector::ForwardVector;
    FVector Right = Request.Right.GetSafeNormal2D();
    if (Right.IsNearlyZero()) Right = FVector(-Forward.Y, Forward.X, 0.f);

    const int32 UnitCount = Units.Num();
    const float Spacing = CalculateMinimumSpacing(Units);
    const bool bDrawnFront = Request.FrontWidth > KINDA_SMALL_NUMBER;
    int32 SlotsPerRow = bDrawnFront ? FMath::Max(1, FMath::FloorToInt(Request.FrontWidth / Spacing) + 1) : 1;
    int32 Rows = ResolveRows(UnitCount, Request.RequestedRows, SlotsPerRow, bDrawnFront);
    TArray<FFormationSlot> Slots;

    if (Request.Formation == ETFFormation::Column && !bDrawnFront)
    {
        // A column stays narrow; the row setting controls the number of parallel files.
        SlotsPerRow = Request.RequestedRows > 0 ? Request.RequestedRows : FMath::Clamp(FMath::CeilToInt(FMath::Sqrt(static_cast<float>(UnitCount))), 1, 3);
        Rows = FMath::DivideAndRoundUp(UnitCount, SlotsPerRow);
    }
    else if (!bDrawnFront)
    {
        SlotsPerRow = FMath::DivideAndRoundUp(UnitCount, Rows);
    }

    if (bDrawnFront)
    {
        SlotsPerRow = FMath::DivideAndRoundUp(UnitCount, Rows);
        Plan.FrontWidthUsed = FMath::Max(Request.FrontWidth, (SlotsPerRow - 1) * Spacing);
    }
    else
    {
        Plan.FrontWidthUsed = (SlotsPerRow - 1) * Spacing;
    }

    if (Request.Formation == ETFFormation::Wedge && !bDrawnFront)
    {
        TArray<int32> RowCounts;
        RowCounts.SetNumZeroed(Rows);
        int32 Remaining = UnitCount;
        const int32 WeightTotal = Rows * (Rows + 1) / 2;
        for (int32 Row = 0; Row < Rows; ++Row)
        {
            const int32 Weight = Row + 1;
            RowCounts[Row] = FMath::Max(1, FMath::FloorToInt(static_cast<float>(UnitCount) * Weight / WeightTotal));
            Remaining -= RowCounts[Row];
        }
        for (int32 Row = Rows - 1; Remaining > 0; Row = (Row - 1 + Rows) % Rows, --Remaining) ++RowCounts[Row];
        for (int32 Row = 0; Row < Rows; ++Row)
        {
            AddCenteredRow(Slots, Request.Anchor - Forward * Row * Spacing, Right, RowCounts[Row], Spacing, Row);
        }
    }
    else
    {
        int32 SlotIndex = 0;
        for (int32 Row = 0; Row < Rows && SlotIndex < UnitCount; ++Row)
        {
            const int32 RowCount = FMath::Min(SlotsPerRow, UnitCount - SlotIndex);
            AddCenteredRow(Slots, Request.Anchor - Forward * Row * Spacing, Right, RowCount, Spacing, Row);
            SlotIndex += RowCount;
        }
    }

    TArray<int32> SortedUnits;
    TArray<int32> SortedSlots;
    for (int32 Index = 0; Index < UnitCount; ++Index) { SortedUnits.Add(Index); SortedSlots.Add(Index); }
    SortedUnits.Sort([&Units](const int32 A, const int32 B)
    {
        if (!FMath::IsNearlyEqual(Units[A].WeaponRange, Units[B].WeaponRange, 1.f)) return Units[A].WeaponRange < Units[B].WeaponRange;
        return A < B;
    });
    SortedSlots.Sort([&Slots, &Request, &Forward](const int32 A, const int32 B)
    {
        const float DepthA = FVector::DotProduct(Slots[A].Location - Request.Anchor, Forward);
        const float DepthB = FVector::DotProduct(Slots[B].Location - Request.Anchor, Forward);
        if (!FMath::IsNearlyEqual(DepthA, DepthB)) return DepthA > DepthB;
        return Slots[A].Row < Slots[B].Row;
    });

    Plan.Targets.SetNum(UnitCount);
    for (int32 Start = 0; Start < UnitCount; )
    {
        int32 End = Start + 1;
        while (End < UnitCount && FMath::IsNearlyEqual(Units[SortedUnits[Start]].WeaponRange, Units[SortedUnits[End]].WeaponRange, 1.f)) ++End;
        TArray<int32> GroupUnits;
        TArray<int32> GroupSlots;
        for (int32 Index = Start; Index < End; ++Index) { GroupUnits.Add(SortedUnits[Index]); GroupSlots.Add(SortedSlots[Index]); }
        GroupUnits.Sort([&Units, &Right](const int32 A, const int32 B) { return FVector::DotProduct(Units[A].SourceLocation, Right) < FVector::DotProduct(Units[B].SourceLocation, Right); });
        GroupSlots.Sort([&Slots, &Right](const int32 A, const int32 B) { return FVector::DotProduct(Slots[A].Location, Right) < FVector::DotProduct(Slots[B].Location, Right); });
        for (int32 Index = 0; Index < GroupUnits.Num(); ++Index) Plan.Targets[GroupUnits[Index]] = Slots[GroupSlots[Index]].Location;
        Start = End;
    }

    Plan.RowsUsed = Rows;
    Plan.MinimumSpacing = Spacing;
    return Plan;
}
