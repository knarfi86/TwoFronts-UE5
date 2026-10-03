using UnrealBuildTool;
public class TwoFrontsEditorTarget : TargetRules
{
    public TwoFrontsEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("TwoFronts");
    }
}
