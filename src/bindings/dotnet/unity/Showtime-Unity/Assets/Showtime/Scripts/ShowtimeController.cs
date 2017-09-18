using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ShowtimeController : MonoBehaviour {

	public string stageAddress = "127.0.0.1";
	public string localPerformerName = "unity_performer";

    //Entities
	private ZstComponent root;
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
		Showtime.init ();

		//Join the performance
		Showtime.join (stageAddress);

        //Create and attach callbacks for Showtime events
        entityArrive = new EntityArrivingCallback();
        entityLeave = new EntityLeavingCallback();
        plugArrive = new PlugArrivingCallback();
        plugLeave = new PlugLeavingCallback();
        cableArrive = new CableArrivingCallback();
        cableLeave = new CableLeavingCallback();
        Showtime.attach_entity_arriving_callback(entityArrive);
        Showtime.attach_entity_leaving_callback(entityLeave);
        Showtime.attach_plug_arriving_callback(plugArrive);
        Showtime.attach_plug_leaving_callback(plugLeave);
        Showtime.attach_cable_arriving_callback(cableArrive);
        Showtime.attach_cable_leaving_callback(cableLeave);

        //Create a root entity to represent this client. I recommend 1 per process.
        root = new ZstComponent("ROOT", localPerformerName);
        root.activate();
        add = new AddFilter(root);
        pushA = new Push("addend", root);
        pushB = new Push("augend", root);
        sink = new Sink("sink", root);

        //Connect the plugs together
        Showtime.connect_cable(pushA.plug.get_URI(), add.augend().get_URI());
        Showtime.connect_cable(pushB.plug.get_URI(), add.addend().get_URI());
        Showtime.connect_cable(add.sum().get_URI(), sink.plug.get_URI());

        //Need to wait whilst plugs connect before we send anything. Will need to put some flag into 
        //the plug to signify connection status
        System.Threading.Thread.Sleep(100);

        //Send a value through this plug. This is an Int plug so we send an int (duh)
        pushA.send(27);
        pushB.send(3);
	}
	
	void Update () {
        if (Showtime.is_connected())
            Showtime.poll_once();
    }

	//Clean up on exit. 
	void OnApplicationQuit(){
		Debug.Log ("Cleaning up Showtime");
		Showtime.destroy ();
	}

	//This event loop will block until it fills up from the ZstEndpoint thread.
	//By delaying execution, we can let the main Unity thread take ownership
	IEnumerator ShowtimeEventLoop(){
		while (true) {
			//Wait until queue has at least 1 event
			yield return new WaitUntil (() => Showtime.event_queue_size () > 0);
            Showtime.poll_once();
		}
	}


    // Entities
    //---------
    public class Sink : ZstComponent
    {
        public ZstInputPlug plug;

        public Sink(string name, ZstEntityBase parent) : base("SINK", name, parent)
        {
            activate();
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

        public Push(string name, ZstEntityBase parent) : base("PUSH", name, parent)
        {
            activate();
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
    public class EntityArrivingCallback : ZstEntityEventCallback
    {
        public override void run(ZstEntityBase entity)
        {
            Debug.Log("Entity arriving: " + entity.URI().path());
        }
    }

    public class EntityLeavingCallback : ZstEntityEventCallback
    {
        public override void run(ZstEntityBase entity)
        {
            Debug.Log("Entity leaving: " + entity.URI().path());
        }
    }

    public class CableArrivingCallback : ZstCableEventCallback
    {
        public override void run(ZstCable cable)
        {
            Debug.Log("Cable arriving: " + cable.get_output().path() + " -> " + cable.get_input().path());
        }
    }

    public class CableLeavingCallback : ZstCableEventCallback
    {
        public override void run(ZstCable cable)
        {
            Debug.Log("Cable leaving: " + cable.get_output().path() + " -> " + cable.get_input().path());
        }
    }

    public class PlugArrivingCallback : ZstPlugEventCallback
    {
        public override void run(ZstURI plug)
        {
            Debug.Log("Plug arriving: " + plug.path());
        }
    }

    public class PlugLeavingCallback : ZstPlugEventCallback
    {
        public override void run(ZstURI plug)
        {
            Debug.Log("Plug leaving: " + plug.path());
        }
    }
}
