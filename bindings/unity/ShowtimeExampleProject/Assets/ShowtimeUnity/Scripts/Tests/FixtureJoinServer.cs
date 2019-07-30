using System.Diagnostics;
using UnityEditor;
using UnityEngine;
using UnityEngine.TestTools;

namespace Showtime.Tests
{
    public class FixtureJoinServer : FixtureCreateServer
    {
        public const int test_timeout = 1000;
        public string server_address = $"127.0.0.1:{showtime.STAGE_ROUTER_PORT}";
        public bool autoconnect = true;
        public bool join_by_name = true;

        public override void Setup()
        {
            base.Setup();

            //Create event loop
            EditorApplication.update += Update;

            //Start library
            showtime.init("TestUnity", true);

            if (!autoconnect)
                return;

            if (join_by_name)
            {
                showtime.join_by_name(server_name);
            } else
            {
                showtime.join(server_address);
            }

        }

        public override void Cleanup()
        {
            EditorApplication.update -= Update;
            showtime.destroy();
            base.Cleanup();
        }

        public void Update()
        {
            if (showtime.is_connected())
                UnityEngine.Debug.Log("In update");
                showtime.poll_once();
        }
    }
}
