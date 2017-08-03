using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ShowtimeController : MonoBehaviour {

	public string stageAddress = "127.0.0.1";
	public string localPerformerName = "unity_performer";
	private ZstPatch root;
    private ZstFilter filter;
    private AddFilter add_filter;

    private ZstOutputPlug augend_out;
    private ZstOutputPlug addend_out;
    private ZstInputPlug sum_in;

    //Callbacks
    private EntityArrivingCallback entityArrive;
    private EntityLeavingCallback entityLeave;
    private PlugArrivingCallback plugArrive;
    private PlugLeavingCallback plugLeave;
    private PlugDataCallback plugData;
    private CableArrivingCallback cableArrive;
    private CableLeavingCallback cableLeave;

    // Use this for initialization
    void Start () {

        //Initialise Showtime
		Showtime.init ();

		//Join the performance
		Showtime.join (stageAddress);

        //Create callbacks for Showtime events
        entityArrive = new EntityArrivingCallback();
        entityLeave = new EntityLeavingCallback();
        plugArrive = new PlugArrivingCallback();
        plugLeave = new PlugLeavingCallback();
        plugData = new PlugDataCallback();
        cableArrive = new CableArrivingCallback();
        cableLeave = new CableLeavingCallback();
        Showtime.attach_entity_arriving_callback(entityArrive);
        Showtime.attach_entity_leaving_callback(entityLeave);
        Showtime.attach_plug_arriving_callback(plugArrive);
        Showtime.attach_plug_leaving_callback(plugLeave);
        Showtime.attach_cable_arriving_callback(cableArrive);
        Showtime.attach_cable_leaving_callback(cableLeave);

        //Start the event loop coroutine to listen for showtime events
        //StartCoroutine("ShowtimeEventLoop");

        //Create a performer to represent this client. I recommend 1 per process.
        root = new ZstPatch(localPerformerName);
        filter = new ZstFilter("test_filter", root);
        add_filter = new AddFilter(root);
        
        //Create our local plug objects. Will block until the stage returns them. Could be async?
        augend_out = filter.create_output_plug("augend_out", ZstValueType.ZST_INT);
        addend_out = filter.create_output_plug("addend_out", ZstValueType.ZST_INT);
        sum_in = filter.create_input_plug("sum_in", ZstValueType.ZST_INT);
        sum_in.attach_receive_callback(plugData);

		//Connect the plugs together
		Showtime.connect_cable(augend_out.get_URI(), add_filter.augend().get_URI());
        Showtime.connect_cable(addend_out.get_URI(), add_filter.addend().get_URI());
        Showtime.connect_cable(add_filter.sum().get_URI(), sum_in.get_URI());

        //Need to wait whilst plugs connect before we send anything. Will need to put some flag into 
        //the plug to signify connection status
        System.Threading.Thread.Sleep(100);

        //Send a value through this plug. THis is an Int plug so we send an int (duh)
        augend_out.value().append_int(27);
        addend_out.value().append_int(3);
        augend_out.fire();
        addend_out.fire();

        //Pause again to give the message time to do a round trip internally
        System.Threading.Thread.Sleep(100);
        Debug.Log ("Final plug value: " + sum_in.value().int_at(0));
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


    // Callbacks
    // ---------
    public class EntityArrivingCallback : ZstEntityEventCallback
    {
        public override void run(ZstURI perf)
        {
            Debug.Log("Performer arriving: " + perf.path());
        }
    }

    public class EntityLeavingCallback : ZstEntityEventCallback
    {
        public override void run(ZstURI perf)
        {
            Debug.Log("performer leaving: " + perf.path());
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

    public class PlugDataCallback : ZstPlugDataEventCallback
    {
        public override void run(ZstInputPlug plug)
        {
            Debug.Log("Plug : " + plug.get_URI().path() + " received hit with val " + plug.value().int_at(0));
        }
    }
}
