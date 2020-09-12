// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System;
using System.IO;
using System.Collections.Generic;

public class ShowtimeRuntime : ModuleRules
{
	public ShowtimeRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		//Add any macros that need to be set
		PublicDefinitions.AddRange(
			new string[] {
				"ZST_IMPORT_CORE_API",
				"ZST_IMPORT_CLIENT_API",
				"ZST_IMPORT_SERVER_API",
				"ZST_IMPORT_PLUGIN_API"
		});

		// C runtime flags
		bUseRTTI = false;
		bEnableExceptions = true;

		bool bUseDebug = true;

		// Add any include paths for the plugin
		PublicIncludePaths.AddRange(new string[]{
			Path.Combine(ModuleDirectory, "Public"),
			Path.Combine(PluginDirectory, "external", "include")
		});
		PrivateIncludePaths.AddRange( new string[] {
			//"/Source/Runtime/Launch/Private",
			Path.Combine(ModuleDirectory, "Private")
		});
		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine"
		});
		PrivateDependencyModuleNames.AddRange(new string[] { "Core" });

		var win_lib_path = Path.Combine(PluginDirectory, "external", "lib", "Win64");
		var win_bin_path = Path.Combine(PluginDirectory, "external", "bin", "Win64");
		var win64_libs = new string[]{};
		var win64_binaries = new string[]{};
		if(bUseDebug){
			win64_libs = new string[]{
				Path.Combine(win_lib_path, "Showtimed.lib")
			};

			win64_binaries = new string[]{
				Path.Combine(win_bin_path, "Showtimed.dll")
			};
		} else {
			win64_libs = new string[]{
				Path.Combine(win_lib_path, "Showtime.lib")
			};

			win64_binaries = new string[]{
				Path.Combine(win_bin_path, "Showtime.dll")
			};
		}

		var mac_lib_path = Path.Combine(PluginDirectory, "external", "lib", "Mac");
		var mac_libraries = new string[]{
			Path.Combine(mac_lib_path, "Showtime")
		};

		var android_lib_path = Path.Combine(PluginDirectory, "external", "lib", "Android");
		var android_libraries = new string[]{
			Path.Combine(android_lib_path, "libShowtime.so")
		};

		// Add any import libraries or static libraries
		string[] platform_libs = new string[]{};
		string[] platform_binaries = new string[]{};

		if (Target.Platform == UnrealTargetPlatform.Win64){
			string binariesDir = Path.Combine(PluginDirectory, "Binaries", "Win64");
			platform_libs = win64_libs;
			platform_binaries = win64_binaries;

			// Copy binaries to plugin binary folder for the editor
			if (!Directory.Exists(binariesDir))
				Directory.CreateDirectory(binariesDir);
			foreach (var dll in platform_binaries)
				File.Copy(dll, Path.Combine(binariesDir, Path.GetFileName(dll)), true);
		}

		if (Target.Platform == UnrealTargetPlatform.Mac){
			platform_libs = mac_libraries;
			platform_binaries = mac_libraries;
		}

		if (Target.Platform == UnrealTargetPlatform.Android){
			platform_libs = android_libraries;
			platform_binaries = android_libraries;
			string PluginPath = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);
			AdditionalPropertiesForReceipt.Add("AndroidPlugin", System.IO.Path.Combine(PluginPath, "ShowtimeUnreal_UPL.xml"));
			PublicDependencyModuleNames.AddRange(new string[] {"Launch"});
		}
		PublicAdditionalLibraries.AddRange(platform_libs);

		// Add runtime libraries that need to be bundled alongside the plugin
		List<string> binaries = new List<string>();
		binaries.AddRange(platform_binaries);
		get_platform_plugins();
		//binaries.AddRange(get_platform_plugins());
		foreach (var path in binaries){
			RuntimeDependencies.Add(Path.Combine("$(TargetOutputDir)", Path.GetFileName(path)), path);
		}
	}

	private string[] get_platform_plugins()
	{
		string[] plugins = null;
		string[] files = new string[] { };
        try {
			files = Directory.GetFiles(Path.Combine(PluginDirectory, "external", "bin", Target.Platform.ToString(), "plugins"));
		} catch (System.IO.DirectoryNotFoundException){}

		foreach(var path in files){
			RuntimeDependencies.Add(Path.Combine("$(TargetOutputDir)", "plugins", Path.GetFileName(path)), path);
		}

		return files;
	}
}
