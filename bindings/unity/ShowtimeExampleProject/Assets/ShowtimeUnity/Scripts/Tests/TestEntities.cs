using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.TestTools;
using NUnit.Framework;

namespace Showtime.Tests
{

    public class TestEntities : TestBase
    {
        [Test]
        public void CreateEntitySync()
        {
            var component = new TestComponent("testComponentSync");
            showtime.activate_entity(component);
            Assert.IsTrue(component.is_activated());
            showtime.deactivate_entity(component);
            Assert.IsFalse(component.is_activated());
        }

        [UnityTest]
        public IEnumerator CreateEntityAsync()
        {
            var component = new TestComponent("testComponentAsync");

            showtime.activate_entity_async(component);
            yield return new WaitUntil(() => component.is_activated());

            showtime.deactivate_entity_async(component);
            yield return new WaitUntil(() => !component.is_activated());
        }

        [UnityTest]
        public IEnumerator SynchronisableAdaptor()
        {
            var component = new TestComponent("testComponentAdaptor");
            var sync_adaptor = new TestSynchronisableAdaptor();
            component.add_adaptor(sync_adaptor);

            showtime.activate_entity_async(component);
            yield return new WaitUntil(() => sync_adaptor.activated);

            showtime.deactivate_entity_async(component);
            yield return new WaitUntil(() => !sync_adaptor.activated);
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
