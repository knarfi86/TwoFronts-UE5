#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "TFGameMode.h"
#include "TFFactionDefinition.h"
#include "TFUnitDefinition.h"
#include "TFHealthComponent.h"

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
#endif
