using System.Threading;
using System.Collections;
using System.Collections.Generic;
using NUnit.Framework;
using UnityEditor;
using UnityEngine;
using UnityEngine.TestTools;

namespace Showtime.Tests
{
    public class TestStartup : TestBase
    {
        public override void Setup()
        {
            autoconnect = false;
            base.Setup();
        }

        [Test]
        public void ConnectSync()
        {
            showtime.join("127.0.0.1");
            Assert.IsTrue(showtime.is_connected());
            showtime.leave();
            Assert.IsFalse(showtime.is_connected());
        }

        [UnityTest]
        public IEnumerator ConnectAsync()
        {
            showtime.join_async("127.0.0.1");
            yield return new WaitUntil(() => showtime.is_connected());
            Assert.IsTrue(showtime.is_connected());
            showtime.leave();
            yield return new WaitUntil(() => !showtime.is_connected());
            Assert.IsFalse(showtime.is_connected());
        }

        [UnityTest]
        public IEnumerator ConnectionWatcher()
        {
            bool connected = false;
            showtime.session_events().on_connected_to_stage_events += () => connected = true;
            showtime.session_events().on_disconnected_from_stage_events += () => connected = false;

            showtime.join_async("127.0.0.1");
            yield return new WaitUntil(() => connected);

            showtime.leave();
            yield return new WaitUntil(() => !connected);
        }
    }
}

