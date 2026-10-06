using UnrealBuildTool;
public class TwoFronts : ModuleRules
{
    public TwoFronts(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore", "NavigationSystem", "AIModule", "UMG", "EnhancedInput", "Slate", "SlateCore", "Niagara" });
    }
}
