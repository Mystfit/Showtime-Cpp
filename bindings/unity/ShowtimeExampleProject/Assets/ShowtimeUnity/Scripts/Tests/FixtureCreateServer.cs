using System.Diagnostics;
using UnityEditor;
using UnityEngine;
using UnityEngine.TestTools;

namespace Showtime.Tests
{
    public class FixtureCreateServer : IPrebuildSetup, IPostBuildCleanup
    {
        public bool create_server = true;

        private static ServerHandle server;
        public static string server_name = "test_server";

        public virtual void Setup()
        {
            if (create_server)
                server = showtime.create_server(server_name, showtime.STAGE_ROUTER_PORT);
        }

        public virtual void Cleanup()
        {
            if (create_server)
                showtime.destroy_server(server);
        }
    }
}
