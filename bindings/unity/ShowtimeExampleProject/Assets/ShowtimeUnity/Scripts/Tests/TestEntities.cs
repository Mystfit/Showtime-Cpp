using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.TestTools;
using NUnit.Framework;

namespace Showtime.Tests
{

    public class TestEntities : IPrebuildSetup, IPostBuildCleanup
    {
        public static FixtureJoinServer fixture;

        public void Setup()
        {
            fixture = new FixtureJoinServer("entities");
        }

        public void Cleanup()
        {
            fixture.Cleanup();
        }

        [Test]
        public void CreateEntitySync()
        {
            var component = new TestComponent("testComponentSync");
            fixture.client.get_root().add_child(component);
            Assert.IsTrue(component.is_activated());
            fixture.client.deactivate_entity(component);
            Assert.IsFalse(component.is_activated());
        }

        [UnityTest]
        public IEnumerator CreateEntityAsync()
        {
            var component = new TestComponent("testComponentAsync");

            fixture.client.get_root().add_child(component);
            yield return null;
            Assert.IsTrue(component.is_activated());

            fixture.client.deactivate_entity_async(component);
            yield return null;
            Assert.IsFalse(component.is_activated());
        }

        [UnityTest]
        public IEnumerator SynchronisableAdaptor()
        {
            var component = new TestComponent("testComponentAdaptor");
            var sync_adaptor = new TestSynchronisableAdaptor();
            component.add_adaptor(sync_adaptor);

            fixture.client.get_root().add_child(component);
            yield return null;
            Assert.IsTrue(component.is_activated());

            fixture.client.deactivate_entity_async(component);
            yield return null;
            Assert.IsFalse(component.is_activated());
        }
    }

    class TestComponent : ZstComponent
    {
        public bool activated = false;

        public TestComponent(string path) : base(path)
        {
        }

        public override void on_activation()
        {
            activated = true;
        }

        public override void on_deactivation()
        {
            activated = false;
        }
    }

    class TestSynchronisableAdaptor : ZstSynchronisableAdaptor
    {
        public bool activated = false;

        public override void on_synchronisable_activated(ZstSynchronisable synchronisable)
        {
            activated = true;
        }

        public override void on_synchronisable_deactivated(ZstSynchronisable synchronisable)
        {
            activated = false;
        }
    }
}
