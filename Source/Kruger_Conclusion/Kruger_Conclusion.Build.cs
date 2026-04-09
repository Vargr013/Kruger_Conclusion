// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Kruger_Conclusion : ModuleRules
{
	public Kruger_Conclusion(ReadOnlyTargetRules Target) : base(Target)
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
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"Kruger_Conclusion",
			"Kruger_Conclusion/Poaching Patrol",
			"Kruger_Conclusion/Poaching Patrol/AI",
			"Kruger_Conclusion/Poaching Patrol/Characters",
			"Kruger_Conclusion/Poaching Patrol/Data",
			"Kruger_Conclusion/Variant_Horror",
			"Kruger_Conclusion/Variant_Horror/UI",
			"Kruger_Conclusion/Variant_Shooter",
			"Kruger_Conclusion/Variant_Shooter/AI",
			"Kruger_Conclusion/Variant_Shooter/UI",
			"Kruger_Conclusion/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
