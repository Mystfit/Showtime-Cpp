using System;
using System.Diagnostics;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

    
public class TestOutputComponent : ZstComponent
{
    public ZstOutputPlug output;

    public TestOutputComponent(string path) : base(path)
    {
        output = create_output_plug("out", ZstValueType.ZST_INT);
    }

    public override void Dispose()
    {
        Console.WriteLine("In TestOutputComponent dispose");
        base.Dispose();
    }

    public void send(int val)
    {
        output.append_int(val);
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
        input = create_input_plug("in", ZstValueType.ZST_INT);
    }

    public override void Dispose()
    {
        Console.WriteLine("In TestInputComponent dispose");
        base.Dispose();
    }

    public override void compute(ZstInputPlug plug)
    {
        //showtime.app(LogLevel.notification, String.Format("Received plug hit from {0} with value {1}", plug.URI().path(), plug.float_at(0)));
        Console.WriteLine(String.Format("Received plug hit from {0} with value {1}", plug.URI().path(), plug.float_at(0)));
        last_val_received = plug.int_at(0);
        wait.Set();
    }
}


public class TestHierarchyAdaptor : ZstHierarchyAdaptor
{
    public readonly EventWaitHandle ent_wait = new AutoResetEvent(false);
    public readonly EventWaitHandle perf_wait = new AutoResetEvent(false);

    public string last_arrived_entity;
    public string last_left_entity;
    public string last_arrived_performer;
    public string last_left_performer;

    public override void on_entity_arriving(ZstEntityBase entity)
    {
        last_arrived_entity = entity.URI().ToString();
        ent_wait.Set();
    }

    public override void on_entity_leaving(ZstEntityBase entity)
    {
        last_left_entity = entity.URI().ToString();
        ent_wait.Set();
    }

    public override void on_performer_arriving(ZstPerformer performer)
    {
        last_arrived_performer = performer.URI().ToString();
        perf_wait.Set();
    }

    public override void on_performer_leaving(ZstPerformer performer)
    {
        last_left_performer = performer.URI().ToString();
        perf_wait.Set();
    }
}


public class TestPlugEchoAdaptor : ZstSynchronisableAdaptor
{
    public readonly EventWaitHandle val_wait = new AutoResetEvent(false);

    public override void on_synchronisable_updated(ZstSynchronisable synchronisable)
    {
        showtime.app(LogLevel.notification, "Synchronisable received updated");
        val_wait.Set();
    }
}


public class Program
{
    public static CancellationTokenSource _cancelationTokenSource;

    static void TestGraph()
    {
        //Create entities
        var input_comp = new TestInputComponent("test_input_comp");
        var output_comp = new TestOutputComponent("test_output_comp");

        //Activate input component
        showtime.activate_entity(input_comp);

        //Activate output component
        showtime.activate_entity(output_comp);

        //Connect cable
        var cable = showtime.connect_cable(input_comp.input, output_comp.output);

        //Send values
        int send_val = 42;
        output_comp.send(send_val);

        //Wait for value to be received
        input_comp.wait.WaitOne();
        Debug.Assert(input_comp.last_val_received == send_val);
        input_comp.wait.Reset();

        //Clean up entities
        showtime.deactivate_entity(input_comp);
        showtime.deactivate_entity(output_comp);
    }
    static void TestExternalEntities()
    {
        //Create adaptors and entities
        TestHierarchyAdaptor adp = new TestHierarchyAdaptor();
        showtime.add_hierarchy_adaptor(adp);

        TestOutputComponent output = new TestOutputComponent("sink_comm_out");
        showtime.activate_entity(output);
        
        //Create sink process
        ProcessStartInfo sink_startInfo = new ProcessStartInfo();
        sink_startInfo.UseShellExecute = false;
        sink_startInfo.RedirectStandardInput = true;
        sink_startInfo.FileName = "TestHelperSink.exe";
        sink_startInfo.Arguments = "a";   // Put sink into test mode

        Process sink_process = new Process();
        sink_process.StartInfo = sink_startInfo;
        sink_process.Start();

        //Wait for performer
        adp.perf_wait.WaitOne();
        adp.perf_wait.Reset();

        //Wait for incoming entity events
        showtime.app(LogLevel.notification, "Waiting for entity to arrive");
        adp.ent_wait.WaitOne();
        Debug.Assert(adp.last_arrived_entity == "sink/sink_ent");
        adp.ent_wait.Reset();
        ZstEntityBase sink_in_ent = showtime.find_entity(new ZstURI("sink/sink_ent/in"));
        Debug.Assert(sink_in_ent != null);
        ZstInputPlug sink_in = showtime.cast_to_input_plug(sink_in_ent);

        //Attach adaptors
        TestPlugEchoAdaptor plug_watcher = new TestPlugEchoAdaptor();
        ZstOutputPlug sink_out_plug = showtime.cast_to_output_plug(showtime.find_entity(new ZstURI("sink/sink_ent/out")));
        sink_out_plug.add_adaptor(plug_watcher);
        showtime.observe_entity(sink_out_plug);

        //Create cable to sink
        showtime.connect_cable(sink_in, output.output);

        //Ask sink to create entity
        output.send(1);

        //Wait for events
        adp.ent_wait.WaitOne();
        Debug.Assert(plug_watcher.val_wait.WaitOne(1000));
        plug_watcher.val_wait.Reset();

        //Make sure entity arrived
        ZstURI sinkB = new ZstURI("sink/sink_ent/sinkB");
        Debug.Assert(adp.last_arrived_entity == sinkB.ToString());
        Debug.Assert(showtime.find_entity(sinkB) != null);
        adp.ent_wait.Reset();

        //Ask sink to remove entity
        output.send(2);
        adp.ent_wait.WaitOne();
        Debug.Assert(adp.last_left_entity == "sink/sink_ent/sinkB");
        Debug.Assert(showtime.find_entity(sinkB) == null);
        adp.ent_wait.Reset();

        //Ask sink to leave
        output.send(0);
        adp.perf_wait.WaitOne();
        showtime.app(LogLevel.notification, adp.last_left_performer);
        Debug.Assert(adp.last_left_performer == "sink");
        Debug.Assert(showtime.find_entity(new ZstURI("sink")) == null);
        adp.perf_wait.Reset();

        //Cleanup
        showtime.deactivate_entity(output);
        showtime.remove_hierarchy_adaptor(adp);
    }

    static int Main(string[] args)
    {
        ProcessStartInfo server_startInfo = new ProcessStartInfo();

        //Required to redirect standard input/output
        server_startInfo.UseShellExecute = false; 
        server_startInfo.RedirectStandardInput = true;
        server_startInfo.FileName = "ShowtimeServer.exe";
        server_startInfo.Arguments = "-t";   // Put server into test mode

        Process server_process = new Process();
        server_process.StartInfo = server_startInfo;
        server_process.Start();
        
        Console.WriteLine("Starting TestDotnet");

        //Start the library
        showtime.init("TestDotnet", true);
        showtime.init_file_logging("showtime.log");

        //Create the event loop
        _cancelationTokenSource = new CancellationTokenSource();
        var eventloop = new Task(() => event_loop(), _cancelationTokenSource.Token, TaskCreationOptions.AttachedToParent);
        eventloop.Start();

        //Join the stage
        showtime.join("127.0.0.1");

        //Run tests
        TestGraph();
        TestExternalEntities();

        //Stop the event loop
        _cancelationTokenSource.Cancel();
        eventloop.Wait();

        //Destroy the library
        showtime.destroy();

        //Kill the stage
        server_process.StandardInput.WriteLine("$TERM\n");
        server_process.WaitForExit();

        Console.WriteLine("Test completed");
        return 0;
    }


    public static void event_loop()
    {
        while (!_cancelationTokenSource.Token.IsCancellationRequested)
        {
            showtime.poll_once();
        }
    }
}

