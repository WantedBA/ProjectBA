// Copyright TeamBA. All Rights Reserved.

using UnrealBuildTool;

public class BAProjectEditor : ModuleRules
{
	public BAProjectEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"BAProject",
			"AIModule",
			"BehaviorTreeEditor",
			"Blutility",
			"Niagara",
			"NiagaraEditor",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"InputCore",
			"UnrealEd",
			"ToolMenus",
			"EditorFramework",
			"EditorStyle",
			"EditorSubsystem",
			"AssetTools",
			"AssetRegistry",
			"Json",
			"JsonUtilities",
			"Projects",
		});
	}
}
