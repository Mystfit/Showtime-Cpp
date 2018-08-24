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
    public GameObject proxy_prefab;

    private string m_client_name;
    private ConnectionWatcher m_connection_watcher;

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
        m_client_name = client;
    }

    void Start()
    {
        m_connection_watcher = new ConnectionWatcher();
    }

    void Update()
    {
        if (showtime.is_connected())
        {
            showtime.poll_once();
        }
    }

    public void Connect()
    {
        showtime.init(m_client_name, true);
        showtime.init_file_logging("unity-showtime.log");

        m_connection_watcher.connected_dlg += OnConnected;
        m_connection_watcher.connected_dlg += OnDisconnected;
        showtime.add_session_adaptor(m_connection_watcher);

        showtime.join_async(address);
    }

    public void OnConnected()
    {
        TransformableEntityWatcher entity_watcher = gameObject.AddComponent<TransformableEntityWatcher>();
        entity_watcher.transformable_prefab = proxy_prefab;
    }

    public void OnDisconnected()
    {
        Destroy(GetComponent< TransformableEntityWatcher>());
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


public class ConnectionWatcher : ZstSessionAdaptor
{
    public delegate void ConnectionWatcherDlg();
    public ConnectionWatcherDlg connected_dlg;
    public ConnectionWatcherDlg disconnected_dlg;
    
    public override void on_connected_to_stage()
    {
        connected_dlg();
    }

    public override void on_disconnected_from_stage()
    {
        disconnected_dlg();
    }
}
