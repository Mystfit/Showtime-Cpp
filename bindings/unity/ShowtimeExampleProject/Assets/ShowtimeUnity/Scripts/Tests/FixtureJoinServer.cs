using System.Diagnostics;
using UnityEditor;
using UnityEngine;
using UnityEngine.TestTools;

namespace Showtime.Tests
{
    public class FixtureJoinServer
    {
        public const int test_timeout = 1000;

        public ShowtimeClient client;
        public ShowtimeServer server;

        public FixtureJoinServer(string test_name)
        {
            server = new ShowtimeServer(test_name);
            client = new ShowtimeClient();

            //Create event loop
            EditorApplication.update += Update;

            //Start library
            client.init("TestUnity", true);
            client.auto_join_by_name(test_name);
        }

        public void Cleanup()
        {
            EditorApplication.update -= Update;
            client.destroy();
            server.destroy();
        }

        public void Update()
        {
            if (client.is_connected())
                UnityEngine.Debug.Log("In update");
            client.poll_once();
        }
    }
}
