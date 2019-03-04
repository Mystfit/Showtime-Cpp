using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using UnityEngine;

public class TestServerStarter
{
    public static showtime.ShowtimeServer server;

    public void Start()
    {
        zst_create_server("unity", 40004);
    }

    public void Stop()
    {
        server_process?.StandardInput.WriteLine("$TERM\n");
        server_process?.WaitForExit();
        server_process = null;
    }
}
