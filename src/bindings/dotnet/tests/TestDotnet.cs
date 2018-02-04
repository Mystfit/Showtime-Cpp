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


public class TestShowtimeDotnet
{
    private static CancellationTokenSource _cancelationTokenSource;

    public TestShowtimeDotnet()
    {
        Console.WriteLine("Starting TestDotnet");

        //Start the library
        showtime.init("TestDotnet", false, "");

        //Create the event loop
        _cancelationTokenSource = new CancellationTokenSource();
        new Task(() => event_loop(), _cancelationTokenSource.Token, TaskCreationOptions.LongRunning).Start();

        //Join the stage
        showtime.join("127.0.0.1");
    
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
        output_comp.send(42.0f);

        //Clean up entities
        showtime.deactivate_entity(input_comp);
        showtime.deactivate_entity(output_comp);
        
        //Stop the event loop
        _cancelationTokenSource.Cancel();

        //Leave the stage and clean up
        showtime.destroy();
    }

    public void event_loop()
    {
        while (!_cancelationTokenSource.Token.IsCancellationRequested)
        {
            showtime.poll_once();
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

