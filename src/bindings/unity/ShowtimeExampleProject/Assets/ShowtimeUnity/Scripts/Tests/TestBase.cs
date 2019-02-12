using System.Diagnostics;
using UnityEditor;
using UnityEngine;
using UnityEngine.TestTools;

namespace Showtime.Tests
{
    public class TestBase : IPrebuildSetup, IPostBuildCleanup
    {
        public static Process server_process;
        public bool autoconnect = true;

        public virtual void Setup()
        {
            StartServer();

            //Create event loop
            EditorApplication.update += Update;

            //Start library
            showtime.init("TestUnity", true);

            if (autoconnect)
                showtime.join("127.0.0.1");
        }

        public void StartServer()
        {
            var server_startInfo = new ProcessStartInfo
            {
                //Required to redirect standard input/output
                UseShellExecute = false,
                RedirectStandardInput = true,
                FileName = $"{Application.dataPath}/ShowtimeUnity/plugins/x86_64/ShowtimeServer",
                Arguments = "-t"   // Put server into test mode
            };

            server_process = Process.Start(server_startInfo);
        }

        public void Cleanup()
        {
            showtime.destroy();
            server_process?.StandardInput.WriteLine("$TERM\n");
            server_process?.WaitForExit();
            server_process = null;
        }

        public void Update()
        {
            if (showtime.is_connected())
                showtime.poll_once();
        }
    }
}
