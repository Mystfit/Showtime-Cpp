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
    private StageCallback stageCallback;
    private PlugCallback plugCallback;

	// Use this for initialization
	void Start () {

		Showtime.init ();


        stageCallback = new StageCallback();
        Showtime.attach_stage_event_callback(stageCallback);

		//Start the event loop coroutine to listen for showtime events

		//Join the performance
		Showtime.join (stageAddress);

		//Create a performer to represent this client. I recommend 1 per process.
		localPerformer = Showtime.create_performer (localPerformerName);

		//Harcoded URIs describing the plugs we own
		ZstURI local_uri_out = new ZstURI(localPerformerName, "ins", "plug_out");
		ZstURI local_uri_in = new ZstURI(localPerformerName, "ins", "plug_in");

		//Create our local plug objects. Will block until the stage returns them. Could be async?
		local_plug_out = Showtime.create_output_plug(local_uri_out, ZstValueType.ZST_INT);
		local_plug_in = Showtime.create_input_plug(local_uri_in, ZstValueType.ZST_INT);

        plugCallback = new PlugCallback();
        local_plug_in.input_events().attach_event_callback(plugCallback);

		//Connect the plugs together
		Showtime.connect_cable(local_uri_out, local_uri_in);
        Showtime.poll_once();

        //Need to wait whilst plugs connect before we send anything. Will need to put some flag into 
        //the plug to signify connection status
        System.Threading.Thread.Sleep(100);

        //Send a value through this plug. THis is an Int plug so we send an int (duh)
        local_plug_out.value().append_int(27);
		local_plug_out.fire();

		//Pause again to give the message time to do a round trip internally
		System.Threading.Thread.Sleep(100);
        StartCoroutine ("ShowtimeEventLoop");
        Debug.Log ("Final plug value: " + local_plug_in.value().int_at(0));
	}
	
	void Update () {
		
	}

	//Clean up on exit. Currently this is broken when calling Unity playmode more than once due
	//to the Showtime singleton persisting in memory. Will implement a leave() function to fix this.
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

    public class PlugCallback : ZstInputPlugEventCallback{
        public override void run(ZstInputPlug plug)
        {
            Debug.Log("Plug: " + plug.get_URI().to_char() + " received hit with val " + plug.value().int_at(0));
        }
    }


    public class StageCallback : ZstEventCallback
    {
        public override void run(ZstEvent e)
        {
            Debug.Log("Stage: " + e.get_update_type().ToString());
        }
    }
}
