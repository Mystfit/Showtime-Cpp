using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ShowtimeController : MonoBehaviour {

	public string stageAddress = "127.0.0.1";
	public string localPerformerName = "unity_performer";
	private SWIGTYPE_p_ZstPerformer localPerformer;
	private ZstOutputPlug local_plug_out;
	private ZstInputPlug local_plug_in;

    //Callbacks
    private PerformerArrivingCallback perfArrive;
    private PerformerLeavingCallback perfLeave;
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
        perfArrive = new PerformerArrivingCallback();
        perfLeave = new PerformerLeavingCallback();
        plugArrive = new PlugArrivingCallback();
        plugLeave = new PlugLeavingCallback();
        plugData = new PlugDataCallback();
        cableArrive = new CableArrivingCallback();
        cableLeave = new CableLeavingCallback();
        Showtime.attach_performer_arriving_callback(perfArrive);
        Showtime.attach_performer_leaving_callback(perfLeave);
        Showtime.attach_plug_arriving_callback(plugArrive);
        Showtime.attach_plug_leaving_callback(plugLeave);
        Showtime.attach_cable_arriving_callback(cableArrive);
        Showtime.attach_cable_leaving_callback(cableLeave);

        //Start the event loop coroutine to listen for showtime events
        //StartCoroutine("ShowtimeEventLoop");

        //Create a performer to represent this client. I recommend 1 per process.
        localPerformer = Showtime.create_performer (localPerformerName);

		//Harcoded URIs describing the plugs we own
		ZstURI local_uri_out = new ZstURI(localPerformerName, "ins", "plug_out");
		ZstURI local_uri_in = new ZstURI(localPerformerName, "ins", "plug_in");

		//Create our local plug objects. Will block until the stage returns them. Could be async?
		local_plug_out = Showtime.create_output_plug(local_uri_out, ZstValueType.ZST_INT);
		local_plug_in = Showtime.create_input_plug(local_uri_in, ZstValueType.ZST_INT);
        local_plug_in.input_events().attach_event_callback(plugData);

		//Connect the plugs together
		Showtime.connect_cable(local_uri_out, local_uri_in);

        //Need to wait whilst plugs connect before we send anything. Will need to put some flag into 
        //the plug to signify connection status
        System.Threading.Thread.Sleep(100);

        //Send a value through this plug. THis is an Int plug so we send an int (duh)
        local_plug_out.value().append_int(27);
		local_plug_out.fire();

		//Pause again to give the message time to do a round trip internally
		System.Threading.Thread.Sleep(100);
        Debug.Log ("Final plug value: " + local_plug_in.value().int_at(0));
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
    public class PerformerArrivingCallback : ZstPerformerEventCallback
    {
        public override void run(ZstURI perf)
        {
            Debug.Log("Performer arriving: " + perf.to_char());
        }
    }

    public class PerformerLeavingCallback : ZstPerformerEventCallback
    {
        public override void run(ZstURI perf)
        {
            Debug.Log("performer leaving: " + perf.to_char());
        }
    }

    public class CableArrivingCallback : ZstCableEventCallback
    {
        public override void run(ZstCable cable)
        {
            Debug.Log("Cable arriving: " + cable.get_output().to_char() + " -> " + cable.get_input().to_char());
        }
    }

    public class CableLeavingCallback : ZstCableEventCallback
    {
        public override void run(ZstCable cable)
        {
            Debug.Log("Cable leaving: " + cable.get_output().to_char() + " -> " + cable.get_input().to_char());
        }
    }

    public class PlugArrivingCallback : ZstPlugEventCallback
    {
        public override void run(ZstURI plug)
        {
            Debug.Log("Plug arriving: " + plug.to_char());
        }
    }

    public class PlugLeavingCallback : ZstPlugEventCallback
    {
        public override void run(ZstURI plug)
        {
            Debug.Log("Plug leaving: " + plug.to_char());
        }
    }

    public class PlugDataCallback : ZstPlugDataEventCallback
    {
        public override void run(ZstInputPlug plug)
        {
            Debug.Log("Plug : " + plug.get_URI().to_char() + " received hit with val " + plug.value().int_at(0));
        }
    }
}
