using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace showtime_dotnet_test
{
    class Callback : PlugCallback
    {
        public override void run(ZstPlug plug)
        {
            Console.WriteLine("Callback ran successfully on plug " + plug.get_URI().name());
        }
    }

    class Program
    {
        static void Main(string[] args)
        {
            Showtime.join("127.0.0.1");
            SWIGTYPE_p_ZstPerformer perf = Showtime.create_performer("dotnet_perf");

            Console.WriteLine("Running local plug test");

            ZstURI local_uri_out = ZstURI.create("dotnet_perf", "ins", "plug_out", ZstURI.Direction.OUT_JACK);
            ZstURI local_uri_in = ZstURI.create("dotnet_perf", "ins", "plug_in", ZstURI.Direction.IN_JACK);

            ZstIntPlug local_plug_out = Showtime.create_int_plug(local_uri_out);
            ZstIntPlug local_plug_in = Showtime.create_int_plug(local_uri_in);

            Callback callback = new Callback();
            local_plug_in.attach_recv_callback(callback);

            Showtime.connect_plugs(local_uri_out, local_uri_in);

            System.Threading.Thread.Sleep(100);

            local_plug_out.fire(1);

            System.Threading.Thread.Sleep(3000);

            Console.WriteLine("Run python remote send test? y/n");
            if(Console.ReadKey().KeyChar == 'y')
            {
                Console.WriteLine("Running python remote plug test");

                ZstURI remote_uri_in = ZstURI.create("python_perf", "ins", "plug_in", ZstURI.Direction.IN_JACK);

                Showtime.connect_plugs(local_uri_out, remote_uri_in);
                System.Threading.Thread.Sleep(200);

                local_plug_out.fire(1);

                System.Threading.Thread.Sleep(3000);
            }


            Console.WriteLine("Run python remote listen test? y/n");
            if (Console.ReadKey().KeyChar == 'y')
            {
                Console.WriteLine("Running python remote plug listen test. Press Esc to exit");
                if (Console.ReadKey().Key == ConsoleKey.Escape)
                    Console.WriteLine("Done");
            }

            Showtime.destroy();
        }
    }
}
