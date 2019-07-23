using System.Diagnostics;
using UnityEditor;
using UnityEngine;
using UnityEngine.TestTools;

namespace Showtime.Tests
{
    public class TestBase : IPrebuildSetup, IPostBuildCleanup
    {
        public static ServerHandle server;
        public bool autoconnect = true;

        public virtual void Setup()
        {
            server = showtime.create_server("unity_server", showtime.STAGE_ROUTER_PORT);

            //Create event loop
            EditorApplication.update += Update;

            //Start library
            showtime.init("TestUnity", true);

            if (autoconnect)
                showtime.join_by_name("unity_server");

        }

        public void Cleanup()
        {
            showtime.destroy();
            showtime.destroy_server(server);
        }

        public void Update()
        {
            if (showtime.is_connected())
                showtime.poll_once();
        }
    }
}
