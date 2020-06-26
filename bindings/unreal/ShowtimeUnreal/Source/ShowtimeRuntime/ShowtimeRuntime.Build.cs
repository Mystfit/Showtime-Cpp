// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
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
		bUseRTTI = true;
		bEnableExceptions = true;

		// Add any include paths for the plugin
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
		
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));
		PrivateIncludePaths.Add(Path.Combine(PluginDirectory, "external", "include"));

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine"
		});
		PrivateDependencyModuleNames.AddRange(new string[] { "Core" });

		var win_lib_path = Path.Combine(PluginDirectory, "external", "lib", "Win64");
		var win_bin_path = Path.Combine(PluginDirectory, "external", "bin", "Win64");
		var win64_libs = new string[]{
			Path.Combine(win_lib_path, "showtime.lib")
		};

		var win64_binaries = new string[]{
			Path.Combine(win_bin_path, "showtime.dll")
		};

		var mac_lib_path = Path.Combine(PluginDirectory, "external", "lib", "Mac");
		var mac_libraries = new string[]{
			Path.Combine(mac_lib_path, "libshowtime.dylib"),
		};

		var android_lib_path = Path.Combine(PluginDirectory, "external", "lib", "arm64-v8a");
		var android_libraries = new string[]{
			Path.Combine(android_lib_path, "libshowtime.so")
		};

		// Add any import libraries or static libraries
		string[] platform_libs = new string[]{};
		string[] platform_binaries = new string[]{};

		if (Target.Platform == UnrealTargetPlatform.Win64){
			string binariesDir = Path.Combine(PluginDirectory, "Binaries", "Win64");
			platform_libs = win64_libs;
			PublicDelayLoadDLLs.AddRange(new string[] { Path.Combine(binariesDir, "showtime.dll") });
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
		}
		PublicAdditionalLibraries.AddRange(platform_libs);

		// Add runtime libraries that need to be bundled alongside the plugin
		List<string> binaries = new List<string>();
		binaries.AddRange(platform_binaries);
		binaries.AddRange(get_platform_plugins());
		foreach (var path in binaries)
			RuntimeDependencies.Add(path);
	}

	private string[] get_platform_plugins()
	{
		string[] plugins = null;
		string[] files = new string[] { };
        try {
			files = Directory.GetFiles(Path.Combine(PluginDirectory, "external", "bin", Target.Platform.ToString(), "plugins"));
		} catch (System.IO.DirectoryNotFoundException){}

		return files;
	}
}
