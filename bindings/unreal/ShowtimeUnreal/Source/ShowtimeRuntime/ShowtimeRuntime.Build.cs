// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System;
using System.IO;
using System.Collections.Generic;

public class ShowtimeRuntime : ModuleRules
{
	public ShowtimeRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		//For C++17 support
		PCHUsage = PCHUsageMode.NoSharedPCHs;
        PrivatePCHHeaderFile = Path.Combine(ModuleDirectory, "Public", "ShowtimePCH.h");

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
		PrivateIncludePaths.AddRange(new string[] {
			Path.Combine(ModuleDirectory, "Private")
		});
		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine"
		});
		PrivateDependencyModuleNames.AddRange(new string[] { "Core" });

		// Architecture
		string arch = "";
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
			arch = Target.WindowsPlatform.Architecture.ToString();
        }
		else if (Target.Platform == UnrealTargetPlatform.Android)
		{
			arch = "arm64-v8a";
		}
		else if (Target.Platform == UnrealTargetPlatform.HoloLens)
        {
			arch = Target.HoloLensPlatform.Architecture.ToString();
		}
		Console.WriteLine(Target.Architecture);

		var win_lib_path = Path.Combine(PluginDirectory, "external", "lib", "Win64", arch);
		var win_bin_path = Path.Combine(PluginDirectory, "external", "bin", "Win64", arch);
		var win64_libs = new string[] { };
		var win64_binaries = new string[] { };

		// Get Win libraries
		if (bUseDebug)
		{
			win64_libs = new string[]{
				Path.Combine(win_lib_path, "Showtimed.lib")
			};

			win64_binaries = new string[]{
				Path.Combine(win_bin_path, "Showtimed.dll")
			};
		}
		else
		{
			win64_libs = new string[]{
				Path.Combine(win_lib_path, "Showtime.lib")
			};

			win64_binaries = new string[]{
				Path.Combine(win_bin_path, "Showtime.dll")
			};
		}

		// Mac paths and libraries
		var mac_lib_path = Path.Combine(PluginDirectory, "external", "lib", "Mac");
		var mac_libraries = new string[]{
			Path.Combine(mac_lib_path, "Showtime")
		};

		// Android paths
		var android_lib_path = Path.Combine(PluginDirectory, "external", "lib", "Android", arch);
		string[] android_libraries = new string[] { };
		try
		{
			// Search for android libraries
			android_libraries = Directory.GetFiles(android_lib_path, "*.so", SearchOption.AllDirectories);
		}
		catch (System.IO.DirectoryNotFoundException) { }

		// Add any import libraries or static libraries
		List<string> platform_libs = new List<string>();
		List<string> platform_binaries = new List<string>();

		// Set windows binaries
		if (Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.HoloLens)
		{
			string binariesDir = Path.Combine(PluginDirectory, "Binaries", "Win64");
			platform_libs.AddRange(win64_libs);
			platform_binaries.AddRange(win64_binaries);

			// Copy binaries to plugin binary folder for the editor
			if (!Directory.Exists(binariesDir))
				Directory.CreateDirectory(binariesDir);
			foreach (var dll in platform_binaries)
				File.Copy(dll, Path.Combine(binariesDir, Path.GetFileName(dll)), true);
		}

		// Set mac binaries
		if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			platform_libs.AddRange(mac_libraries);
			platform_binaries.AddRange(mac_libraries);
		}

		// Set android binaries
		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			// Add fixed android libraries
			platform_libs.AddRange(android_libraries);
			platform_binaries.AddRange(android_libraries);

			// Add android plugins
			//platform_binaries.AddRange(get_platform_plugins(arch));

			// Showtime plugin path
			string PluginPath = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);

			// Add UPL script
			AdditionalPropertiesForReceipt.Add("AndroidPlugin", System.IO.Path.Combine(PluginPath, "ShowtimeUnreal_UPL.xml"));

			// Public modules
			PublicDependencyModuleNames.AddRange(new string[] { "Launch" });

			// External binary source dir for plugin copying
			string binariesDir = Path.Combine(PluginDirectory, "Binaries", "Android", arch);
			
			// Copy binaries to plugin binary folder
			//if (!Directory.Exists(binariesDir))
			//	Directory.CreateDirectory(binariesDir);
			//foreach (var so in platform_binaries)
			//	File.Copy(so, Path.Combine(binariesDir, Path.GetFileName(so)), true);
		}
		PublicAdditionalLibraries.AddRange(platform_libs);

		// Add runtime libraries that need to be bundled alongside the plugin
		List<string> binaries = new List<string>();
		binaries.AddRange(platform_binaries);
		binaries.AddRange(get_platform_plugins(arch));
		foreach (var path in binaries)
		{
			System.Console.WriteLine("Adding runtime dependency: " + path);
			RuntimeDependencies.Add(Path.Combine("$(TargetOutputDir)", Path.GetFileName(path)), path);
		}
	}

	private string[] get_platform_plugins(string architecture)
	{
		string[] plugins = null;
		string[] files = new string[] { };
		var bin_folder = Path.Combine(PluginDirectory, "external", "bin", Target.Platform.ToString(), architecture);
		bin_folder = Path.Combine(bin_folder, "plugins");
		Console.WriteLine("Searching for Showtime plugins in: " + bin_folder);

		try
		{
			files = Directory.GetFiles(bin_folder);
			System.Console.WriteLine("Plugin files:" + string.Join("\n", files));
		}
		catch (System.IO.DirectoryNotFoundException) {
			System.Console.WriteLine("Couldn't find plugin folder " + bin_folder );
		}

		//foreach (var path in files)
		//{
		//	RuntimeDependencies.Add(Path.Combine("$(TargetOutputDir)", "plugins", Path.GetFileName(path)), path);
		//}

		return files;
	}
}
