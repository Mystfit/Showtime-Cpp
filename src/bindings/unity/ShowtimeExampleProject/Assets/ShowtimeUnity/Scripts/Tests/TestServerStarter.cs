using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using UnityEngine;

public class TestServerStarter
{
    public Process server_process;

    public void Start()
    {
        var server_startInfo = new ProcessStartInfo
        {
            //Required to redirect standard input/output
            UseShellExecute = false,
            RedirectStandardInput = true,
            FileName = $"{Application.dataPath}/ShowtimeUnity/plugins/x86_64/ShowtimeServer",
            Arguments = "-t"   // Put server into test mode
        };

        server_process = Process.Start(server_startInfo);
    }

    public void Stop()
    {
        server_process?.StandardInput.WriteLine("$TERM\n");
        server_process?.WaitForExit();
        server_process = null;
    }
}
