using System;
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
        output = create_output_plug("out", ZstValueType.ZST_FLOAT);
    }
       
    public void send(float val)
    {
        output.append_float(val);
        output.fire();
    }
}

public class TestInputComponent : ZstComponent
{
    public ZstInputPlug input;
    public readonly EventWaitHandle wait = new AutoResetEvent(false);

    public TestInputComponent(string path) : base(path)
    {
        input = create_input_plug("in", ZstValueType.ZST_FLOAT);
    }
    public override void compute(ZstInputPlug plug)
    {
        Console.WriteLine(String.Format("Received plug hit from {0} with value {1}", plug.URI().path(), plug.float_at(0)));
        wait.Set();
    }
}

public class PerformerCallback : ZstPerformerEvent
{
    public readonly EventWaitHandle wait = new AutoResetEvent(false);

    private Action<ZstSynchronisable> m_a;
    public PerformerCallback(Action<ZstSynchronisable> a)
    {
        m_a = a;
    }

    public override void run_with_performer(ZstPerformer target)
    {
        m_a.Invoke(target);
        wait.Set();
    }
}

public class EntityCallback : ZstEntityEvent
{
    public readonly EventWaitHandle wait = new AutoResetEvent(false);

    private Action<ZstEntityBase> m_a;
    public EntityCallback(Action<ZstEntityBase> a)
    {
        m_a = a;
    }

    public override void run_with_entity(ZstEntityBase target)
    {
        m_a.Invoke(target);
        wait.Set();
    }
}

public class CableCallback : ZstCableEvent
{
    public readonly EventWaitHandle wait = new AutoResetEvent(false);

    private Action<ZstCable> m_a;
    public CableCallback(Action<ZstCable> a)
    {
        m_a = a;
    }

    public override void run_with_cable(ZstCable target)
    {
        m_a.Invoke(target);
        wait.Set();
    }
}


public class TestShowtimeDotnet
{
    private static CancellationTokenSource _cancelationTokenSource;

    public TestShowtimeDotnet()
    {
        Console.WriteLine("Starting TestDotnet");

        //Start the library
        Showtime.init("TestDotnet");

        //Create the event loop
        _cancelationTokenSource = new CancellationTokenSource();
        new Task(() => event_loop(), _cancelationTokenSource.Token, TaskCreationOptions.LongRunning).Start();

        //Attach a connection listener
        var conn_event = new PerformerCallback((target) => Console.WriteLine("Connected"));
        Showtime.attach_connection_event_listener(conn_event);

        //Join the stage
        Showtime.join("127.0.0.1");

        //Wait for the connection event to finish
        conn_event.wait.WaitOne();
        conn_event.wait.Reset();
        Showtime.remove_connection_event_listener(conn_event);

        //Create entities
        var entity_activated = new EntityCallback((target) => Console.WriteLine(String.Format("Entity {0} activated", target.URI().path())));
        var input_comp = new TestInputComponent("test_input_comp");
        var output_comp = new TestOutputComponent("test_output_comp");

        //Activate input component
        input_comp.attach_activation_event(entity_activated);
        Showtime.activate_entity(input_comp);
        entity_activated.wait.WaitOne();
        entity_activated.wait.Reset();

        //Activate output component
        output_comp.attach_activation_event(entity_activated);
        Showtime.activate_entity(output_comp);
        entity_activated.wait.WaitOne();
        entity_activated.wait.Reset();

        //Connect cable
        var cable_event = new CableCallback((target) => Console.WriteLine("Cable connected"));
        var cable = Showtime.connect_cable(input_comp.input, output_comp.output);
        cable.attach_activation_event(cable_event);
        cable_event.wait.WaitOne();
        cable_event.wait.Reset();

        //Send values
        output_comp.send(42.0f);
        input_comp.wait.WaitOne();
        input_comp.wait.Reset();
        
        //Clean up entities
        Showtime.deactivate_entity(input_comp);
        Showtime.deactivate_entity(output_comp);

        //Clean up listeners
        input_comp.detach_activation_event(entity_activated);
        output_comp.detach_activation_event(entity_activated);
        cable.detach_activation_event(cable_event);

        //Stop the event loop
        _cancelationTokenSource.Cancel();

        //Leave the stage and clean up
        Showtime.destroy();
    }

    public void event_loop()
    {
        while (!_cancelationTokenSource.Token.IsCancellationRequested)
        {
            Showtime.poll_once();
        }
    }
}


public class Program
{
    static void Main(string[] args)
    {
        var test = new TestShowtimeDotnet();
    }
}

