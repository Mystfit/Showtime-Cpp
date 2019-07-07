using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEditor;

namespace Showtime
{
    [ExecuteInEditMode]
    public class PluginPlatforms
    {
        [MenuItem("Showtime/Restore plugin import settings")]
        public static void RestorePluginDefaults()
        {
            var android_plugins = AssetDatabase.FindAssets("libShowtime", new[] { "Assets/ShowtimeUnity/Plugins/Android" });
            foreach (var plugin_GUID in android_plugins)
            {
                string android_plugin_path = AssetDatabase.GUIDToAssetPath(plugin_GUID);
                Debug.Log($"Fixing {android_plugin_path}");
                PluginImporter android_plugin = AssetImporter.GetAtPath(android_plugin_path) as PluginImporter;
                android_plugin.SetCompatibleWithAnyPlatform(false);
                android_plugin.SetCompatibleWithEditor(false);
                android_plugin.SetCompatibleWithPlatform(BuildTarget.Android, true);
                android_plugin.SetPlatformData(BuildTarget.Android, "CPU", "ARM64");
                android_plugin.SaveAndReimport();
            }

            var osx_plugins = AssetDatabase.FindAssets(".bundle", new[] { "Assets/ShowtimeUnity/Plugins/OSX" });
            foreach (var plugin_GUID in osx_plugins)
            {
                string osx_plugin_path = AssetDatabase.GUIDToAssetPath(plugin_GUID);
                Debug.Log($"Fixing {osx_plugin_path}");
                PluginImporter osx_plugin = AssetImporter.GetAtPath(osx_plugin_path) as PluginImporter;
                osx_plugin.SetCompatibleWithAnyPlatform(false);
                osx_plugin.SetCompatibleWithEditor(true);
                osx_plugin.SetCompatibleWithPlatform(BuildTarget.StandaloneOSX, true);
                osx_plugin.SetEditorData("CPU", "x86_64");
                osx_plugin.SetEditorData("OS", "OSX");
                osx_plugin.SetPlatformData(BuildTarget.StandaloneOSX, "x64", "true");
                osx_plugin.SaveAndReimport();
            }

            var windows_plugins = AssetDatabase.FindAssets("Showtime", new[] { "Assets/ShowtimeUnity/Plugins/windows" });
            foreach (var plugin_GUID in windows_plugins)
            {
                string windows_plugin_path = AssetDatabase.GUIDToAssetPath(plugin_GUID);
                Debug.Log($"Fixing {windows_plugin_path}");
                PluginImporter windows_plugin = AssetImporter.GetAtPath(windows_plugin_path) as PluginImporter;

                if (windows_plugin == null)
                    continue;
                if (windows_plugin.isNativePlugin)
                {
                    windows_plugin.SetCompatibleWithAnyPlatform(false);
                    windows_plugin.SetCompatibleWithEditor(true);
                    windows_plugin.SetCompatibleWithPlatform(BuildTarget.StandaloneWindows64, true);
                    windows_plugin.SetEditorData("CPU", "x86_64");
                    windows_plugin.SetEditorData("OS", "Windows");
                    windows_plugin.SaveAndReimport();
                }
            }
        }
    }
}
