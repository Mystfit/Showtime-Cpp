using System.Collections;
using System.Collections.Generic;
using UnityEngine;


public class ExampleObservableManager : MonoBehaviour
{
    //Showtime vars
    public string address = "127.0.0.1";
    public string component_name = "cube";
    public string owner_name;
    public bool is_master = true;
    private string client_name;

    //Setters
    public void SetMaster(bool val)
    {
        is_master = val;
    }

    public void SetOwner(string s)
    {
        owner_name = s;
    }

    public void SetAddress(string stage_address)
    {
        address = stage_address;
    }

    public void SetClientName(string client)
    {
        client_name = client;
    }

    void Update()
    {

        if (showtime.is_connected())
        {
            showtime.poll_once();
            if (is_master)
            {
            }
        }
    }

    public void Connect()
    {
        showtime.init(client_name, true);
        showtime.init_file_logging();
        showtime.join(address);

        if (is_master)
        {
        }
        else
        {
            gameObject.AddComponent<TransformableEntityWatcher>();
        }
    }

    public void Disconnect()
    {
        showtime.leave();
    }

    private void OnApplicationQuit()
    {
        showtime.destroy();
    }
}
