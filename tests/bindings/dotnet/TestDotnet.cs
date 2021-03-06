﻿using NUnit.Framework;
using System;
using System.Threading;
using System.Threading.Tasks;

namespace Showtime.Tests
{
    public class TestOutputComponent : ZstComponent
    {
        public ZstOutputPlug output;

        public TestOutputComponent(string path) : base(path)
        {
            output = new ZstOutputPlug("out", ZstValueType.IntList);
        }

        public override void on_registered()
        {
            add_child(output);
            Assert.IsTrue(output.is_registered());
        }

        public void send(int val)
        {
            output.append_int(val);
            Console.WriteLine("===Sending val===");
            output.fire();
        }
    }

    public class TestInputComponent : ZstComponent
    {
        public ZstInputPlug input;
        public readonly EventWaitHandle wait = new AutoResetEvent(false);
        public float last_val_received = 0.0f;

        public TestInputComponent(string path) : base(path)
        {
            input = new ZstInputPlug("in", ZstValueType.IntList);
        }
        public override void on_registered()
        {
            add_child(input);
            Assert.IsTrue(input.is_registered());
        }

        public override void compute(ZstInputPlug plug)
        {
            //showtime.app(LogLevel.notification, String.Format("Received plug hit from {0} with value {1}", plug.URI().path(), plug.float_at(0)));
            showtime.app(LogLevel.debug, string.Format("Received plug hit from {0} with value {1}", plug.URI().path(), plug.float_at(0)));
            last_val_received = plug.int_at(0);
            wait.Set();
        }
    }

    [TestFixture]
    [NonParallelizable]
    public class FixtureCreateServer
    {
        private ShowtimeServer m_server;

        [SetUp]
        public void SetupServer()
        {
            Console.WriteLine("===CREATE SERVER===");
            m_server = new ShowtimeServer(NUnit.Framework.TestContext.CurrentContext.Test.Name);
        }

        [TearDown]
        public void CleanupServer()
        {
            Console.WriteLine("===DESTROY SERVER===");
            m_server.destroy();
        }
    }

    [TestFixture]
    [NonParallelizable]
    public class FixtureInitLibrary : FixtureCreateServer
    {
        private static Task m_eventloop;
        private static CancellationTokenSource m_cancelationTokenSource;
        public ShowtimeClient client;

        public static void event_loop(ShowtimeClient client)
        {
            while (!m_cancelationTokenSource.Token.IsCancellationRequested)
            {
                client.poll_once();
                Thread.Sleep(10);
            }
        }

        [SetUp]
        public void SetupLibrary()
        {
            Console.WriteLine("===INIT LIBRARY===");

            //Start the library
            client = new ShowtimeClient();
            client.init("TestDotnet", true);
            //client.init_file_logging("showtime.log");

            //Create the event loop
            m_cancelationTokenSource = new CancellationTokenSource();
            m_eventloop = new Task(() => event_loop(client), m_cancelationTokenSource.Token, TaskCreationOptions.AttachedToParent);
            m_eventloop.Start();
        }

        [TearDown]
        public void CleanupLibrary()
        {
            Console.WriteLine("===DESTROY LIBRARY===");

            //Stop the event loop
            m_cancelationTokenSource.Cancel();
            m_eventloop.Wait();

            //Destroy the library
            client.destroy();
        }
    }

    [TestFixture]
    [NonParallelizable]
    public class FixtureJoinServer : FixtureInitLibrary
    {
        [SetUp]
        public void JoinServer()
        {
            //Join the server
            Console.WriteLine("===JOIN SERVER===");

            client.auto_join_by_name(NUnit.Framework.TestContext.CurrentContext.Test.Name);
        }
    }


    public class Graph : FixtureJoinServer
    {
        [Test]
        public void SentThroughReliableGraph()
        {
            //Create entities
            TestInputComponent input_comp = new TestInputComponent("test_input_comp");
            TestOutputComponent output_comp = new TestOutputComponent("test_output_comp");

            //Parent input component
            client.get_root().add_child(input_comp);

            //Parent output component
            client.get_root().add_child(output_comp);

            Assert.IsTrue(input_comp.is_activated());
            Assert.IsTrue(input_comp.input.is_registered());
            Assert.IsTrue(input_comp.input.is_activated());
            Assert.IsTrue(output_comp.is_activated());
            Assert.IsTrue(output_comp.output.is_activated());

            //Connect cable
            ZstCable cable = client.connect_cable(input_comp.input, output_comp.output);
            Assert.IsNotNull(cable);

            //Send values
            int send_val = 42;
            output_comp.send(send_val);

            //Wait for value to be received
            input_comp.wait.WaitOne(1000);
            Assert.AreEqual(send_val, input_comp.last_val_received);
        }
    }

    public class Connection : FixtureInitLibrary
    {
        [Test]
        public void ConnectionEvents()
        {
            EventWaitHandle wait = new AutoResetEvent(false);
            bool connected = false;
            client.connection_events().on_connected_to_stage_events += (client, server) => { connected = true; wait.Set(); };
            client.connection_events().on_disconnected_from_stage_events += (client, server) => { connected = false; wait.Set(); };
            client.auto_join_by_name(NUnit.Framework.TestContext.CurrentContext.Test.Name);

            wait.WaitOne(1000);
            Assert.IsTrue(connected);
        }
    }
}



//public class TestHierarchyAdaptor : ZstHierarchyAdaptor
//{
//    public readonly EventWaitHandle ent_wait = new AutoResetEvent(false);
//    public readonly EventWaitHandle perf_wait = new AutoResetEvent(false);

//    public string last_arrived_entity;
//    public string last_left_entity;
//    public string last_arrived_performer;
//    public string last_left_performer;

//    public override void on_entity_arriving(ZstEntityBase entity)
//    {
//        if (entity != null)
//        {
//            last_arrived_entity = entity.URI().ToString();
//            ent_wait.Set();
//        }
//    }

//    public override void on_entity_leaving(ZstEntityBase entity)
//    {
//        if (entity != null)
//        {
//            last_left_entity = entity.URI().ToString();
//            ent_wait.Set();
//        }
//    }

//    public override void on_performer_arriving(ZstPerformer performer)
//    {
//        if (performer != null)
//        {
//            last_arrived_performer = performer.URI().ToString();
//            perf_wait.Set();
//        }
//    }

//    public override void on_performer_leaving(ZstPerformer performer)
//    {
//        if (performer != null)
//        {
//            last_left_performer = performer.URI().ToString();
//            perf_wait.Set();
//        }
//    }
//}


//public class TestPlugEchoAdaptor : ZstSynchronisableAdaptor
//{
//    public readonly EventWaitHandle val_wait = new AutoResetEvent(false);

//    public override void on_synchronisable_updated(ZstSynchronisable synchronisable)
//    {
//        showtime.app(LogLevel.notification, "Synchronisable received updated");
//        val_wait.Set();
//    }
//}


//public class Program
//{
//    static void TestExternalEntities(string sink_path)
//    {
//        //Create adaptors and entities
//        TestHierarchyAdaptor adp = new TestHierarchyAdaptor();
//        showtime.add_hierarchy_adaptor(adp);

//        TestOutputComponent output = new TestOutputComponent("sink_comm_out");
//        showtime.get_root().add_child(output);

//        //Create sink process
//        ProcessStartInfo sink_startInfo = new ProcessStartInfo();
//        sink_startInfo.UseShellExecute = false;
//        sink_startInfo.RedirectStandardInput = true;
//        sink_startInfo.FileName = sink_path;
//        sink_startInfo.Arguments = "a";   // Put sink into test mode

//        Process sink_process = new Process();
//        sink_process.StartInfo = sink_startInfo;
//        sink_process.Start();

//        //Wait for performer
//        adp.perf_wait.WaitOne();
//        adp.perf_wait.Reset();

//        //Query performer list
//        ZstEntityBundle bundle = new ZstEntityBundle();
//        showtime.get_performers(bundle);
//        Debug.Assert(bundle.Count == 2);

//        //Check bundle is iterable
//        foreach(var p in bundle)
//        {
//            Debug.Assert(p.entity_type() == showtime.PERFORMER_TYPE);
//            showtime.app(LogLevel.debug, "Found performer " + p.URI().ToString());
//        }

//        //Wait for incoming entity events
//        showtime.app(LogLevel.notification, "Waiting for entity to arrive");
//        int count = 5;
//        while(adp.last_arrived_entity != "sink/sink_ent" && --count > 0)
//            adp.ent_wait.WaitOne();
//        Debug.Assert(adp.last_arrived_entity == "sink/sink_ent");
//        adp.ent_wait.Reset();
//        ZstEntityBase sink_in_ent = showtime.find_entity(new ZstURI("sink/sink_ent/in"));
//        Debug.Assert(sink_in_ent != null);
//        ZstInputPlug sink_in = showtime.cast_to_input_plug(sink_in_ent);

//        //Attach adaptors
//        TestPlugEchoAdaptor plug_watcher = new TestPlugEchoAdaptor();
//        ZstOutputPlug sink_out_plug = showtime.cast_to_output_plug(showtime.find_entity(new ZstURI("sink/sink_ent/out")));
//        sink_out_plug.add_adaptor(plug_watcher);
//        showtime.observe_entity(sink_out_plug);

//        //Create cable to sink
//        showtime.connect_cable(sink_in, output.output);

//        //Ask sink to create entity
//        output.send(1);

//        //Wait for events
//        adp.ent_wait.WaitOne();
//        Debug.Assert(plug_watcher.val_wait.WaitOne(1000));
//        plug_watcher.val_wait.Reset();

//        //Make sure entity arrived
//        ZstURI sinkB = new ZstURI("sink/sink_ent/sinkB");
//        Debug.Assert(adp.last_arrived_entity == sinkB.ToString());
//        Debug.Assert(showtime.find_entity(sinkB) != null);
//        adp.ent_wait.Reset();

//        //Ask sink to remove entity
//        output.send(2);
//        adp.ent_wait.WaitOne();
//        Debug.Assert(adp.last_left_entity == "sink/sink_ent/sinkB");
//        Debug.Assert(showtime.find_entity(sinkB) == null);
//        adp.ent_wait.Reset();

//        //Ask sink to leave
//        output.send(0);
//        adp.perf_wait.WaitOne();
//        showtime.app(LogLevel.notification, adp.last_left_performer);
//        Debug.Assert(adp.last_left_performer == "sink");
//        Debug.Assert(showtime.find_entity(new ZstURI("sink")) == null);
//        adp.perf_wait.Reset();
//    }
//}
