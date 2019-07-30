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
            showtime.init("TestUnity", true);
            showtime.join($"127.0.0.1:{showtime.STAGE_ROUTER_PORT}");
            Assert.IsTrue(showtime.is_connected());
            showtime.leave();
            Assert.IsFalse(showtime.is_connected());
        }

        [UnityTest]
        [PrebuildSetup(typeof(FixtureJoinServer))]
        public IEnumerator ConnectAsync()
        {
            showtime.init("TestUnity", true);
            showtime.join_async($"127.0.0.1:{showtime.STAGE_ROUTER_PORT}");
            yield return new WaitUntil(() => showtime.is_connected());
            Assert.IsTrue(showtime.is_connected());
            showtime.leave();
            yield return null;
            Assert.IsFalse(showtime.is_connected());
        }

        [UnityTest]
        [PrebuildSetup(typeof(FixtureJoinServer))]
        public IEnumerator ConnectionWatcher()
        {
            showtime.init("TestUnity", true);

            bool connected = false;
            showtime.session_events().on_connected_to_stage_events += () => connected = true;
            showtime.session_events().on_disconnected_from_stage_events += () => connected = false;

            showtime.join_async($"127.0.0.1:{showtime.STAGE_ROUTER_PORT}");
            yield return new WaitUntil(()=>showtime.is_connected());
            Assert.IsTrue(connected);

            showtime.leave();
            yield return null;
            Assert.IsFalse(connected);
        }
    }
}

