using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ExampleCableConnectComponent : ZstComponent
{
    Transform m_sphere;
    public ZstInputPlug input;
    public ZstOutputPlug output;

    public ExampleCableConnectComponent(Transform sphere, string name) : base(name)
    {
        input = new ZstInputPlug("input", ZstValueType.FloatList);
        output = new ZstOutputPlug("output", ZstValueType.FloatList);
        m_sphere = sphere;
    }

    public override void on_registered()
    {
        add_child(input);
        add_child(output);
    }

    public void set_position(Vector3 position)
    {
        output.append_float(position.x);
        output.append_float(position.y);
        output.append_float(position.z);
        output.fire();
    }

    public override void compute(ZstInputPlug plug)
    {
        Vector3 pos = new Vector3(plug.float_at(0), plug.float_at(1), plug.float_at(2));
        m_sphere.transform.position = pos;
    }
}


public class ExampleCableConnect : MonoBehaviour {

    //Showtime vars
    public string address = "127.0.0.1";
    public string component_name = "cube";
    public string owner_name;
    public bool is_master = true;
    private string client_name;
    private ExampleCableConnectComponent sphere_component;
    private ShowtimeClient m_client;

    //Anim vars
    public float speed = 5.0f;
    public float amount = 5.0f;

    public void Start()
    {
        m_client = new ShowtimeClient();
    }

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

    public void SetTarget(string t)
    {
        owner_name = t;
    }
    	
	void Update () {

        if (m_client.is_connected())
        {
            m_client.poll_once();
            if (is_master)
            {
                transform.position = new Vector3(0.0f, Mathf.Sin(Time.time * speed) * amount, 0.0f);
                sphere_component.set_position(transform.position);
            }
        }
    }

    public void Connect()
    {
        m_client.init(client_name, true);
        m_client.join(address);
        
        if (is_master)
        {
            sphere_component = new ExampleCableConnectComponent(transform, component_name);
            m_client.get_root().add_child(sphere_component);
        }
        else
        {
            ZstComponent sphere_proxy = showtime.cast_to_component(m_client.find_entity(new ZstURI(owner_name).add(new ZstURI("sphere"))));
            Debug.Log(string.Format("Sphere proxy is {0}", sphere_proxy));

            ZstOutputPlug output_p = showtime.cast_to_output_plug(sphere_proxy.get_child_by_URI(sphere_proxy.URI().add(new ZstURI("output"))));
            Debug.Log(string.Format("Output plug is {0}", output_p));

            sphere_component = new ExampleCableConnectComponent(transform, component_name);
            m_client.get_root().add_child(sphere_component);

            m_client.connect_cable(sphere_component.input, output_p);
        }
    }

    public void Disconnect()
    {
        m_client.leave();
    }

    private void OnApplicationQuit()
    {
        if (m_client.is_connected())
        {
            m_client.deactivate_entity(sphere_component);
            m_client.destroy();
        }
    }
}

// Callbacks
// ---------
public class EntityCallback : ZstHierarchyAdaptor
{
    public override void on_entity_arriving(ZstEntityBase entity)
    {
        Debug.Log("Entity arriving: " + entity.URI().path());
    }

    public override void on_entity_leaving(ZstEntityBase entity)
    {
        Debug.Log("Entity leaving: " + entity.URI().path());
    }

    public override void on_plug_arriving(ZstPlug plug)
    {
        Debug.Log("Plug arriving: " + plug.URI().path());
    }

    public override void on_plug_leaving(ZstPlug plug)
    {
        Debug.Log("Plug leaving: " + plug.URI().path());
    }
}

public class SessionCallback : ZstSessionAdaptor
{
    public override void on_cable_created(ZstCable cable)
    {
        Debug.Log("Cable arriving: " + cable.get_address().get_output_URI(). path() + " -> " + cable.get_address().get_input_URI().path());
    }

    public override void on_cable_destroyed(ZstCable cable)
    {
        Debug.Log("Cable leaving: " + cable.get_address().get_output_URI().path() + " -> " + cable.get_address().get_input_URI().path());
    }
}
