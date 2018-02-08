using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ShowtimeController : MonoBehaviour {

	public string stageAddress = "127.0.0.1";
	public string localPerformerName = "unity_performer";

    //Entities
    private Push pushA;
    private Push pushB;
    private Sink sink;
    private AddFilter add;

    //Callbacks
    private EntityArrivingCallback entityArrive;
    private EntityLeavingCallback entityLeave;
    private PlugArrivingCallback plugArrive;
    private PlugLeavingCallback plugLeave;
    private CableArrivingCallback cableArrive;
    private CableLeavingCallback cableLeave;

    // Use this for initialization
    void Start () {

        //Initialise Showtime
		showtime.init("unity_test", true);
        showtime.init_file_logging("unity_showtime.log");

        //Join the performance
        showtime.join(stageAddress);

        //Create and attach callbacks for Showtime events
        entityArrive = new EntityArrivingCallback();
        entityLeave = new EntityLeavingCallback();
        plugArrive = new PlugArrivingCallback();
        plugLeave = new PlugLeavingCallback();
        cableArrive = new CableArrivingCallback();
        cableLeave = new CableLeavingCallback();

        //Create a root entity to represent this client. I recommend 1 per process.
        
        add = new AddFilter("adder");
        pushA = new Push("addend");
        pushB = new Push("augend");
        sink = new Sink("sink");

        showtime.activate_entity(add);
        showtime.activate_entity(pushA);
        showtime.activate_entity(pushB);
        showtime.activate_entity(sink);

        Debug.Log("adder activated:" + add.is_activated());
        Debug.Log("addend activated:" + pushA.is_activated());
        Debug.Log("augend activated:" + pushB.is_activated());
        Debug.Log("sink activated:" + sink.is_activated());

        //Connect the plugs together
        ZstCable augend_cable = showtime.connect_cable(add.augend(), pushA.plug);
        ZstCable addend_cable = showtime.connect_cable(add.addend(), pushB.plug);
        ZstCable sum_cable = showtime.connect_cable(sink.plug, add.sum());

        //Send a value through this plug. This is an Int plug so we send an int (duh)
        pushA.send(27);
        pushB.send(3);
	}
	
	void Update () {
        if (showtime.is_connected())
            showtime.poll_once();
    }

	//Clean up on exit. 
	void OnApplicationQuit(){
		Debug.Log ("Cleaning up Showtime");
        showtime.deactivate_entity(add);
        showtime.deactivate_entity(pushA);
        showtime.deactivate_entity(pushB);
        showtime.deactivate_entity(sink);
        showtime.destroy ();
	}
    
    // Entities
    //---------
    public class Sink : ZstComponent
    {
        public ZstInputPlug plug;

        public Sink(string name) : base(name)
        {
            plug = create_input_plug("push_in", ZstValueType.ZST_INT);
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
            plug = create_output_plug("push_out", ZstValueType.ZST_INT);
        }

        public void send(int val)
        {
            plug.append_int(val);
            plug.fire();
        }
    }


    // Callbacks
    // ---------
    public class EntityArrivingCallback : ZstComponentEvent
    {
        public override void run_with_component(ZstComponent target)
        {
            Debug.Log("Entity arriving: " + target.URI().path());
        }
    }

    public class EntityLeavingCallback : ZstComponentEvent
    {
        public override void run_with_component(ZstComponent target)
        {
            Debug.Log("Entity leaving: " + target.URI().path());
        }
    }

    public class CableArrivingCallback : ZstCableEvent
    {
        public override void run_with_cable(ZstCable cable)
        {
            Debug.Log("Cable arriving: " + cable.get_output_URI().path() + " -> " + cable.get_input_URI().path());
        }
    }

    public class CableLeavingCallback : ZstCableEvent
    {
        public override void run_with_cable(ZstCable cable)
        {
            Debug.Log("Cable leaving: " + cable.get_output_URI().path() + " -> " + cable.get_input_URI().path());
        }
    }

    public class PlugArrivingCallback : ZstPlugEvent
    {
        public override void run_with_plug(ZstPlug target)
        {
            Debug.Log("Plug arriving: " + target.URI().path());
        }
    }

    public class PlugLeavingCallback : ZstPlugEvent
    {
        public override void run_with_plug(ZstPlug target)
        {
            Debug.Log("Plug leaving: " + target.URI().path());
        }
    }
}
