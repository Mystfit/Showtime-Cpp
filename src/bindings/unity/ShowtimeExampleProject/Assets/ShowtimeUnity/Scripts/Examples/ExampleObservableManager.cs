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
        m_connection_watcher.connected_dlg += OnConnected;
        m_connection_watcher.disconnected_dlg += OnDisconnected;
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
        if(m_client_name == null){
            throw new System.NullReferenceException("Showtime performer name was null");
        }
        showtime.init(m_client_name, true);
        showtime.init_file_logging("unity-showtime.log");
        showtime.add_session_adaptor(m_connection_watcher);
        showtime.join_async(address);
    }

    public void OnConnected()
    {
        Debug.Log("Connected to stage");
        TransformableEntityWatcher entity_watcher = gameObject.AddComponent<TransformableEntityWatcher>();
        entity_watcher.transformable_prefab = proxy_prefab;
    }

    public void OnDisconnected()
    {
        TransformableEntityWatcher watcher = GetComponent< TransformableEntityWatcher>();
        if(watcher != null)
            Destroy(watcher);
    }

    public void Disconnect()
    {
        showtime.leave();
    }

    private void OnApplicationQuit()
    {
        showtime.remove_session_adaptor(m_connection_watcher);
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
        connected_dlg.Invoke();
    }

    public override void on_disconnected_from_stage()
    {
        disconnected_dlg.Invoke();
    }
}
