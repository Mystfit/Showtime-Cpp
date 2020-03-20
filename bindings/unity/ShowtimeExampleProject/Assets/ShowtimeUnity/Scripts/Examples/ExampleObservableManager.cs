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

    public ShowtimeClient client;
    private string m_client_name;

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
        client = new ShowtimeClient();
        client.connection_events().on_connected_to_stage_events += OnConnected;
        client.connection_events().on_disconnected_from_stage_events += OnDisconnected;
    }

    void Update()
    {
        if (client.is_connected())
        {
            client.poll_once();
        }
    }

    public void Connect()
    {
        if(m_client_name == null){
            throw new System.NullReferenceException("Showtime performer name was null");
        }
        client.init(m_client_name, true);
        //m_client.init_file_logging("unity-showtime.log");
        client.join_async(address);
    }

    public void OnConnected(ShowtimeClient client, ZstServerAddress server)
    {
        Debug.Log("Connected to stage");
        TransformableEntityWatcher entity_watcher = gameObject.AddComponent<TransformableEntityWatcher>();
        entity_watcher.transformable_prefab = proxy_prefab;
    }

    public void OnDisconnected(ShowtimeClient client, ZstServerAddress server)
    {
        TransformableEntityWatcher watcher = GetComponent< TransformableEntityWatcher>();
        if(watcher != null)
            Destroy(watcher);
    }

    public void Disconnect()
    {
        client.leave();
    }

    private void OnApplicationQuit()
    {
        client.destroy();
    }
}
