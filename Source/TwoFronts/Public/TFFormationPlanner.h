#pragma once

#include "CoreMinimal.h"
#include "TFTypes.h"

struct FTFFormationUnit
{
    FVector SourceLocation = FVector::ZeroVector;
    float WeaponRange = 0.f;
    float CollisionRadius = 34.f;
};

struct FTFFormationPlanRequest
{
    ETFFormation Formation = ETFFormation::Line;
    int32 RequestedRows = 0; // Zero means automatic.
    FVector Anchor = FVector::ZeroVector;
    FVector Forward = FVector::ForwardVector;
    FVector Right = FVector::RightVector;
    float FrontWidth = 0.f; // A positive value preserves a player-drawn front width.
};

struct FTFFormationPlan
{
    TArray<FVector> Targets; // Matches the input unit order.
    int32 RowsUsed = 0;
    float MinimumSpacing = 0.f;
    float FrontWidthUsed = 0.f;
};

// Pure planner shared by normal orders, drawn-line previews and automation tests.
class TWOFRONTS_API FTFFormationPlanner
{
public:
    static FTFFormationPlan BuildPlan(const TArray<FTFFormationUnit>& Units, const FTFFormationPlanRequest& Request);
    static float CalculateMinimumSpacing(const TArray<FTFFormationUnit>& Units);
};
