using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ExampleShowtimeController : MonoBehaviour {

    public string stageAddress = "127.0.0.1";
    public int stagePort = showtime.STAGE_ROUTER_PORT;
    public string localPerformerName = "unity_performer";

    //Entities
    private Push pushA;
    private Push pushB;
    private Sink sink;
    //private AddFilter add;

    //Callbacks
    private EntityCallback entityCallback;
    private SessionCallback sessionCallback;
    private ShowtimeClient m_client;

    // Use this for initialization
    void Start () {

        m_client = new ShowtimeClient();
        //Initialise Showtime
        m_client.init("unity_test", true);
        //m_client.init_file_logging("unity_showtime.log");

        StartCoroutine(TestLibrary());
    }

   private IEnumerator TestLibrary()
    {
        //Join the performance
        m_client.join_async($"{stageAddress}:{stagePort}");

        yield return new WaitUntil(() => m_client.is_connected());

        //Create and attach callbacks for Showtime events
        entityCallback = new EntityCallback();
        sessionCallback = new SessionCallback();
        m_client.add_session_adaptor(sessionCallback);
        m_client.add_hierarchy_adaptor(entityCallback);

        //add = new AddFilter("adder");
        pushA = new Push("addend");
        pushB = new Push("augend");
        sink = new Sink("sink");

        //Entities need to be activated to become 'live' in the performance
        Debug.Log("Activating entities");
        m_client.get_root().add_child(pushA);
        m_client.get_root().add_child(pushB);
        m_client.get_root().add_child(sink);

        //Connect the plugs together with cables
        Debug.Log("Connecting cables");
        //ZstCable augend_cable = m_client.connect_cable_async(add.augend(), pushA.plug);
        //ZstCable addend_cable = m_client.connect_cable_async(add.addend(), pushB.plug);
        //ZstCable sum_cable = m_client.connect_cable_async(sink.plug, add.sum());
        
        //yield return new WaitUntil(() => 
        //    (augend_cable.is_activated() && 
        //    addend_cable.is_activated() && 
        //    sum_cable.is_activated())
        //);

        //Send a value through this plug. This is an Int plug so we send an int (duh)
        Debug.Log("Sending test values");
        pushA.send(27);
        pushB.send(3);
    }
    
    void Update () {
        if (m_client.is_connected())
            m_client.poll_once();
    }

    //Clean up on exit. 
    void OnApplicationQuit(){
        Debug.Log ("Leaving performance");
        //showtime.deactivate_entity(add);
        m_client.deactivate_entity(pushA);
        m_client.deactivate_entity(pushB);
        m_client.deactivate_entity(sink);

        //We don't have to tear down the library at this point unless we want to run init() again.
        //The showtime singleton will take care of itself on program exit, but we still need to leave
        m_client.leave();
    }
    
    // Entities
    //---------
    public class Sink : ZstComponent
    {
        public ZstInputPlug plug;

        public Sink(string name) : base(name)
        {
            plug = new ZstInputPlug("push_in", ZstValueType.IntList);
        }

        public override void on_registered()
        {
            add_child(plug);
        }

        public override void compute(ZstInputPlug plug)
        {
            try
            {
                Debug.Log("Sink received value of " + plug.int_at(0));
            } catch (Exception e)
            {
                Debug.Log(e.ToString());
            }
        }
    }


    public class Push : ZstComponent
    {
        public ZstOutputPlug plug;

        public Push(string name) : base(name)
        {
            plug = new ZstOutputPlug("push_out", ZstValueType.IntList);
        }

        public override void on_registered()
        {
            add_child(plug);
        }

        public void send(int val)
        {
            plug.append_int(val);
            plug.fire();
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
            Debug.Log("Cable arriving: " + cable.get_address().get_output_URI().path() + " -> " + cable.get_address().get_input_URI().path());
        }

        public override void on_cable_destroyed(ZstCable cable)
        {
            Debug.Log("Cable leaving: " + cable.get_address().get_output_URI().path() + " -> " + cable.get_address().get_input_URI().path());
        }
    }
}
