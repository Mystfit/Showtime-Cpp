using System.Threading;
using System.Collections;
using System.Collections.Generic;
using NUnit.Framework;
using UnityEditor;
using UnityEngine;
using UnityEngine.TestTools;

namespace Showtime.Tests
{
    public class TestStartup
    {
        [Test]
        public void ConnectSync()
        {
            ShowtimeServer server = new ShowtimeServer("ConnectSync");
            ShowtimeClient client = new ShowtimeClient();

            client.init("TestUnity", true);
            client.auto_join_by_name(TestContext.CurrentContext.Test.Name);
            Assert.IsTrue(client.is_connected());
            client.leave();
            Assert.IsFalse(client.is_connected());

            client.destroy();
            server.destroy();
        }

        [UnityTest]
        public IEnumerator ConnectAsync()
        {
            ShowtimeServer server = new ShowtimeServer("ConnectAsync");
            ShowtimeClient client = new ShowtimeClient();

            client.init("TestUnity", true);
            client.join_async($"127.0.0.1:{server.port()}");
            yield return new WaitUntil(() => client.is_connected());
            Assert.IsTrue(client.is_connected());
            client.leave();
            yield return null;
            Assert.IsFalse(client.is_connected());

            client.destroy();
            server.destroy();
        }

        [UnityTest]
        public IEnumerator ConnectionWatcher()
        {
            ShowtimeServer server = new ShowtimeServer("ConnectionWatcher");
            ShowtimeClient client = new ShowtimeClient();

            client.init("TestUnity", true);

            bool connected = false;
            client.connection_events().on_connected_to_stage_events += (target_client, server_address) => connected = true;
            client.connection_events().on_disconnected_from_stage_events += (target_client, server_address) => connected = false;

            client.join_async($"127.0.0.1:{server.port()}");
            yield return new WaitUntil(()=>client.is_connected());
            Assert.IsTrue(connected);

            client.leave();
            yield return null;
            Assert.IsFalse(connected);

            client.destroy();
            server.destroy();
        }
    }
}

