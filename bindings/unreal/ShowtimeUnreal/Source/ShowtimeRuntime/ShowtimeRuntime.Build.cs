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
			Path.Combine(win_lib_path, "ShowtimeCore.lib"),
			Path.Combine(win_lib_path, "ShowtimeClient.lib"),
			Path.Combine(win_lib_path, "ShowtimeServer.lib")
		};

		var win64_binaries = new string[]{
			Path.Combine(win_bin_path, "ShowtimeCore.dll"),
			Path.Combine(win_bin_path, "ShowtimeClient.dll"),
			Path.Combine(win_bin_path, "ShowtimeServer.dll"),
			Path.Combine(win_bin_path, "libzmq-v142-mt-4_3_3.dll"),
			Path.Combine(win_bin_path, "libczmq.dll")
		};

		var mac_lib_path = Path.Combine(PluginDirectory, "external", "lib", "Mac");
		var mac_libraries = new string[]{
			Path.Combine(mac_lib_path, "ShowtimeClient"),
			Path.Combine(mac_lib_path, "ShowtimeCore"),
			Path.Combine(mac_lib_path, "ShowtimeServer")
		};

		var android_lib_path = Path.Combine(PluginDirectory, "external", "lib", "arm64-v8a");
		var android_libraries = new string[]{
			Path.Combine(android_lib_path, "ShowtimeClient"),
			Path.Combine(android_lib_path, "ShowtimeCore"),
			Path.Combine(android_lib_path, "ShowtimeServer")
		};

		// Add any import libraries or static libraries
		string[] platform_libs = null;
		string[] platform_binaries = null;

		if (Target.Platform == UnrealTargetPlatform.Win64){
			platform_libs = win64_libs;
			platform_binaries = win64_binaries;

			// Copy binaries to plugin binary folder for the editor
			string binariesDir = Path.Combine(PluginDirectory, "Binaries", "Win64");
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
		string platform = "";
		switch(Target.Platform){
			case UnrealTargetPlatform.Win64:
				platform = "Win64";
				break;
			case UnrealTargetPlatform.Mac:
				platform = "Mac";
				break;
			case UnrealTargetPlatform.Android:
				platform = "arm64-v8a";
				break;
			case default:
				break;
		}

		return Directory.GetFiles(Path.Combine(PluginDirectory, "external", "bin", platform, "plugins"));
	}
}