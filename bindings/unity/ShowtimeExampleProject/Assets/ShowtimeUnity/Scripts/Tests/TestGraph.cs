using System.Collections;
using System.Collections.Generic;
using NUnit.Framework;
using UnityEngine;
using UnityEngine.TestTools;

namespace Showtime.Tests
{
    public class TestGraph : IPrebuildSetup, IPostBuildCleanup
    {
        private FixtureJoinServer fixture;

        public void Setup()
        {
            fixture = new FixtureJoinServer("graph");
        }

        public void Cleanup()
        {
            fixture.Cleanup();
        }

        [UnityTest]
        public IEnumerator SendIntValue()
        {
            var output = new TestGraphOutput("test_output");
            var input = new TestGraphInput("test_input");
            fixture.client.get_root().add_child(output);
            fixture.client.get_root().add_child(input);
            fixture.client.connect_cable(input.plug, output.plug);
    
            var val = 42;
            output.send(val);
            yield return null;
            Assert.IsTrue(input.plug.int_at(0) == val);
        }
    }

    class TestGraphOutput : ZstComponent
    {
        public bool activated = false;
        public ZstOutputPlug plug;

        public TestGraphOutput(string path) : base(path)
        {
            plug = new ZstOutputPlug("int_out", ZstValueType.IntList);
        }

        public override void on_registered()
        {
            add_child(plug);
        }

        public void send(int val)
        {
            plug.append_int(val);
            plug.fire();
        }
    }

    class TestGraphInput : ZstComponent
    {
        public bool dirty = false;
        public ZstInputPlug plug;

        public TestGraphInput(string path) : base(path)
        {
            plug = new ZstInputPlug("int_in", ZstValueType.IntList);
        }

        public override void on_registered()
        {
            add_child(plug);
        }

        public override void compute(ZstInputPlug plug)
        {
            dirty = true;
        }
    }
}
