// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class SpaceRoxEditorTarget : TargetRules
{
	public SpaceRoxEditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_1;
		ExtraModuleNames.Add("SpaceRox");

		// Uncomment these to build without Unity and PCH to detect missing #include directives
		//bUseUnityBuild = false;
		//bUsePCHFiles = false;
    }
}
