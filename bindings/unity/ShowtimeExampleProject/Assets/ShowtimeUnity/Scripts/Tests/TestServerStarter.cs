using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using UnityEngine;

public class TestServerStarter
{
    public static ServerHandle server;

    public void Start()
    {
        server = showtime.create_server("unity", showtime.STAGE_ROUTER_PORT);
    }

    public void Stop()
    {
        showtime.destroy_server(server);
    }
}
