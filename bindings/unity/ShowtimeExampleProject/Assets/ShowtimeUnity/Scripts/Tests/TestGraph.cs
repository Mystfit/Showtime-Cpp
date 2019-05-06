using System.Collections;
using System.Collections.Generic;
using NUnit.Framework;
using UnityEngine;
using UnityEngine.TestTools;

namespace Showtime.Tests
{
    public class TestGraph : TestBase, IPrebuildSetup, IPostBuildCleanup
    {
        [UnityTest]
        public IEnumerator SendIntValue()
        {
            var output = new TestGraphOutput("test_output");
            var input = new TestGraphInput("test_input");
            showtime.activate_entity(output);
            showtime.activate_entity(input);
            var cable = showtime.connect_cable(input.plug, output.plug);
            Assert.IsNotNull(cable);

            var val = 42;
            output.send(val);
            yield return new WaitUntil(() => input.dirty);
            Assert.IsTrue(input.plug.int_at(0) == val);
        }
    }

    class TestGraphOutput : ZstComponent
    {
        public bool activated = false;
        public ZstOutputPlug plug;

        public TestGraphOutput(string path) : base(path)
        {
            plug = create_output_plug("int_out", ZstValueType.ZST_INT);
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
            plug = create_input_plug("int_in", ZstValueType.ZST_INT);
        }

        public override void compute(ZstInputPlug plug)
        {
            dirty = true;
        }
    }
}
