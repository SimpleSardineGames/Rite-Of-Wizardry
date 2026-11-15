// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ROWDEVBUILD : ModuleRules
{
	public ROWDEVBUILD(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"Niagara",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"ROWDEVBUILD",
			"ROWDEVBUILD/Variant_Strategy",
			"ROWDEVBUILD/Variant_Strategy/UI",
			"ROWDEVBUILD/Variant_TwinStick",
			"ROWDEVBUILD/Variant_TwinStick/AI",
			"ROWDEVBUILD/Variant_TwinStick/Gameplay",
			"ROWDEVBUILD/Variant_TwinStick/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
