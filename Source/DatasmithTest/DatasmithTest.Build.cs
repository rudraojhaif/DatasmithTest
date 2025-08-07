// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DatasmithTest : ModuleRules
{
	public DatasmithTest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" , "UMG" ,    "Json",
			"JsonUtilities", "Sockets", "Networking",});

        // Datasmith Runtime Dependencies
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "DatasmithRuntime",
            "DatasmithCore",
            "DatasmithContent",
            "PhysicsCore"
        });

        PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Uncomment if you are using Slate UI
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
