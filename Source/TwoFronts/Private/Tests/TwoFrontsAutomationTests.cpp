#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "TFGameMode.h"
#include "TFFactionDefinition.h"
#include "TFUnitDefinition.h"
#include "TFHealthComponent.h"
#include "TFFormationPlanner.h"
#include "TFDamageSystem.h"
#include "TFWeaponDefinition.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTFPrototypeCatalogTest, "TwoFronts.Gameplay.PrototypeCatalog", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTFPrototypeCatalogTest::RunTest(const FString&)
{
    ATFGameMode* Mode = GetMutableDefault<ATFGameMode>();
    Mode->CreateRuntimeDefinitions();
    const UTFFactionDefinition* Humans = Mode->GetFactionDefinition(ETFactionId::Humans);
    const UTFFactionDefinition* Synth = Mode->GetFactionDefinition(ETFactionId::Synth);
    TestNotNull(TEXT("Humans exists"), Humans); TestNotNull(TEXT("Synth exists"), Synth);
    TestEqual(TEXT("Humans has four production roles"), Humans->Units.Num(), 4);
    TestEqual(TEXT("Synth has four production roles"), Synth->Units.Num(), 4);
    for (int32 Index = 0; Index < 4; ++Index)
    {
        TestEqual(TEXT("Human definition faction matches"), Humans->Units[Index]->Faction, ETFactionId::Humans);
        TestEqual(TEXT("Synth definition faction matches"), Synth->Units[Index]->Faction, ETFactionId::Synth);
        TestEqual(TEXT("Role order matches both factions"), Humans->Units[Index]->Role, Synth->Units[Index]->Role);
        TestTrue(TEXT("Production time remains active"), Humans->Units[Index]->ProductionSeconds > 0.f && Synth->Units[Index]->ProductionSeconds > 0.f);
    }
    TestTrue(TEXT("Scout is faster than heavy Human unit"), Humans->Units[0]->MoveSpeed > Humans->Units[2]->MoveSpeed);
    TestTrue(TEXT("Probe is faster than Walker"), Synth->Units[0]->MoveSpeed > Synth->Units[2]->MoveSpeed);
    TestTrue(TEXT("Human support repairs"), Humans->Units[3]->RepairPerSecond > 0.f);
    TestTrue(TEXT("Synth support repairs"), Synth->Units[3]->RepairPerSecond > 0.f);
    TestNotEqual(TEXT("Faction combat definitions are not cloned"), Humans->Units[1]->Combat.Cooldown, Synth->Units[1]->Combat.Cooldown);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTFHealthRulesTest, "TwoFronts.Gameplay.HealthAndRepair", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTFHealthRulesTest::RunTest(const FString&)
{
    UTFHealthComponent* Health = NewObject<UTFHealthComponent>(GetTransientPackage());
    Health->Initialise(100.f);
    TestEqual(TEXT("Damage is limited by current health"), Health->ApplyDamage(35.f), 35.f);
    TestEqual(TEXT("Damage lowers health"), Health->CurrentHealth, 65.f);
    TestEqual(TEXT("Repair returns applied amount"), Health->Repair(20.f), 20.f);
    TestEqual(TEXT("Repair does not exceed max health"), Health->Repair(100.f), 15.f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTFFormationPlannerTest, "TwoFronts.Gameplay.FormationPlanner", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTFFormationPlannerTest::RunTest(const FString&)
{
    TArray<FTFFormationUnit> Units;
    for (int32 Index = 0; Index < 10; ++Index)
    {
        FTFFormationUnit& Unit = Units.AddDefaulted_GetRef();
        Unit.SourceLocation = FVector(0.f, (Index - 4.5f) * 100.f, 0.f);
        Unit.WeaponRange = 400.f + Index * 50.f;
        Unit.CollisionRadius = Index == 0 ? 55.f : 35.f;
    }
    FTFFormationPlanRequest Request;
    Request.Formation = ETFFormation::Line;
    Request.RequestedRows = 2;
    Request.Anchor = FVector(1000.f, 0.f, 0.f);
    Request.Forward = FVector::ForwardVector;
    Request.Right = FVector(0.f, 1.f, 0.f);
    const FTFFormationPlan Plan = FTFFormationPlanner::BuildPlan(Units, Request);
    TestEqual(TEXT("Manual row count is respected"), Plan.RowsUsed, 2);
    TestEqual(TEXT("Every unit has one target"), Plan.Targets.Num(), Units.Num());
    TestTrue(TEXT("Shortest weapon range is placed furthest forward"), Plan.Targets[0].X > Plan.Targets.Last().X);
    for (int32 A = 0; A < Plan.Targets.Num(); ++A)
    {
        for (int32 B = A + 1; B < Plan.Targets.Num(); ++B)
        {
            TestTrue(FString::Printf(TEXT("Targets %d and %d respect minimum spacing"), A, B), FVector::Dist2D(Plan.Targets[A], Plan.Targets[B]) >= Plan.MinimumSpacing - KINDA_SMALL_NUMBER);
        }
    }

    TArray<FTFFormationUnit> EqualRangeUnits;
    for (int32 Index = 0; Index < 3; ++Index)
    {
        FTFFormationUnit& Unit = EqualRangeUnits.AddDefaulted_GetRef();
        Unit.SourceLocation = FVector(0.f, (Index - 1) * 400.f, 0.f);
        Unit.WeaponRange = 650.f;
        Unit.CollisionRadius = 35.f;
    }
    Request.RequestedRows = 1;
    const FTFFormationPlan EqualRangePlan = FTFFormationPlanner::BuildPlan(EqualRangeUnits, Request);
    TestTrue(TEXT("Equal-range units retain lateral movement order"), EqualRangePlan.Targets[0].Y < EqualRangePlan.Targets[1].Y && EqualRangePlan.Targets[1].Y < EqualRangePlan.Targets[2].Y);

    TArray<FTFFormationUnit> DrawnLineUnits;
    for (int32 Index = 0; Index < 20; ++Index) { FTFFormationUnit& Unit = DrawnLineUnits.AddDefaulted_GetRef(); Unit.SourceLocation = FVector::ZeroVector; Unit.WeaponRange = 600.f; Unit.CollisionRadius = 35.f; }
    Request.RequestedRows = 0;
    Request.FrontWidth = 300.f;
    const FTFFormationPlan DrawnLinePlan = FTFFormationPlanner::BuildPlan(DrawnLineUnits, Request);
    TestTrue(TEXT("A short drawn line automatically creates multiple rows"), DrawnLinePlan.RowsUsed > 1);
    TestTrue(TEXT("A manual row count expands a short drawn front instead of overlapping targets"), FTFFormationPlanner::BuildPlan(DrawnLineUnits, FTFFormationPlanRequest{ETFFormation::Line, 2, Request.Anchor, Request.Forward, Request.Right, 300.f}).FrontWidthUsed >= 9.f * DrawnLinePlan.MinimumSpacing);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTFCombatDataTest, "TwoFronts.Gameplay.CombatData", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTFCombatDataTest::RunTest(const FString&)
{
    ATFGameMode* Mode = GetMutableDefault<ATFGameMode>();
    Mode->CreateRuntimeDefinitions();
    const UTFFactionDefinition* Humans = Mode->GetFactionDefinition(ETFactionId::Humans);
    const UTFFactionDefinition* Synth = Mode->GetFactionDefinition(ETFactionId::Synth);
    TestEqual(TEXT("Armor reduces damage"), FTFDamageSystem::CalculateDamage(20.f, 7.f), 13.f);
    TestEqual(TEXT("Armor cannot erase a positive hit"), FTFDamageSystem::CalculateDamage(4.f, 20.f), 1.f);
    TestEqual(TEXT("Non-positive damage stays zero"), FTFDamageSystem::CalculateDamage(0.f, 20.f), 0.f);
    TestFalse(TEXT("Friendly fire is blocked by default"), FTFDamageSystem::CanApplyDamage(ETFactionId::Humans, ETFactionId::Humans, false));
    TestTrue(TEXT("Enemy damage remains allowed"), FTFDamageSystem::CanApplyDamage(ETFactionId::Humans, ETFactionId::Synth, false));
    TestTrue(TEXT("Rifle uses a direct weapon definition"), Humans->Units[1]->Weapons.Num() == 1 && Humans->Units[1]->Weapons[0]->Delivery == ETFWeaponDelivery::Direct);
    TestTrue(TEXT("Tank uses a projectile weapon definition"), Humans->Units[2]->Weapons.Num() == 1 && Humans->Units[2]->Weapons[0]->Delivery == ETFWeaponDelivery::Projectile);
    TestTrue(TEXT("Walker also uses a projectile weapon definition"), Synth->Units[2]->Weapons.Num() == 1 && Synth->Units[2]->Weapons[0]->Delivery == ETFWeaponDelivery::Projectile);
    TestTrue(TEXT("Weapons declare valid target categories"), Humans->Units[1]->Weapons[0]->ValidTargetCategories.Contains(ETFCombatTargetCategory::Armored));
    const FTFDirectShotVFXSettings& HumanBoltVFX = Humans->Units[1]->Weapons[0]->DirectShotVFX;
    const FTFDirectShotVFXSettings& SynthBoltVFX = Synth->Units[1]->Weapons[0]->DirectShotVFX;
    TestTrue(TEXT("Human bolt uses a warm orange color"), HumanBoltVFX.BoltColor.R > HumanBoltVFX.BoltColor.G && HumanBoltVFX.BoltColor.G > HumanBoltVFX.BoltColor.B);
    TestTrue(TEXT("Synth bolt uses a green color instead of cyan"), SynthBoltVFX.BoltColor.G > SynthBoltVFX.BoltColor.R && SynthBoltVFX.BoltColor.G > SynthBoltVFX.BoltColor.B);
    TestTrue(TEXT("Direct-shot bolt lengths remain compact"), HumanBoltVFX.BoltLength >= 80.f && HumanBoltVFX.BoltLength <= 250.f && SynthBoltVFX.BoltLength >= 80.f && SynthBoltVFX.BoltLength <= 250.f);
    TestTrue(TEXT("Direct-shot bolt speeds use the blaster range"), HumanBoltVFX.BoltSpeed >= 15000.f && HumanBoltVFX.BoltSpeed <= 40000.f && SynthBoltVFX.BoltSpeed >= 15000.f && SynthBoltVFX.BoltSpeed <= 40000.f);
    TestTrue(TEXT("Direct-shot bolts stay shorter than their firing ranges"), HumanBoltVFX.BoltLength < Humans->Units[1]->Weapons[0]->MaximumRange * .5f && SynthBoltVFX.BoltLength < Synth->Units[1]->Weapons[0]->MaximumRange * .5f);
    TestEqual(TEXT("Runtime blaster VFX uses the high-quality profile"), HumanBoltVFX.VFXQuality, ETFLaserVFXQuality::High);
    return true;
}
#endif
