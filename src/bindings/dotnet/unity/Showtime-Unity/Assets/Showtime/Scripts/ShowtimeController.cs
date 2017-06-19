using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ShowtimeController : MonoBehaviour {

	public string stageAddress = "127.0.0.1";
	public string localPerformerName = "unity_performer";
	public bool testPython = false;
	private SWIGTYPE_p_ZstPerformer localPerformer;
	private ZstIntPlug local_plug_out;
	private ZstIntPlug local_plug_in;

	// Use this for initialization
	void Start () {

		Showtime.init ();


		//Start the event loop coroutine to listen for showtime events
		StartCoroutine (ShowtimeEventLoop());

		//Join the performance
		Showtime.join (stageAddress);

		//Create a performer to represent this client. I recommend 1 per process.
		localPerformer = Showtime.create_performer (localPerformerName);

		//Harcoded URIs describing the plugs we own
		ZstURI local_uri_out = ZstURI.create(localPerformerName, "ins", "plug_out", ZstURI.Direction.OUT_JACK);
		ZstURI local_uri_in = ZstURI.create(localPerformerName, "ins", "plug_in", ZstURI.Direction.IN_JACK);

		//Create our local plug objects. Will block until the stage returns them. Could be async?
		local_plug_out = Showtime.create_int_plug(local_uri_out);
		local_plug_in = Showtime.create_int_plug(local_uri_in);

		//Connect the plugs together
		Showtime.connect_plugs(local_uri_out, local_uri_in);

		//Need to wait whilst plugs connect before we send anything. Will need to put some flag into 
		//the plug to signify connection status
		System.Threading.Thread.Sleep(100);

		//Send a value through this plug. THis is an Int plug so we send an int (duh)
		local_plug_out.fire(1);

		//Pause again to give the message time to do a round trip internally
		System.Threading.Thread.Sleep(100);

		Debug.Log ("Final plug value: " + local_plug_in.get_value());

		//Test connecting/sending to python
		if (testPython) {
			ZstURI remote_uri_in = ZstURI.create ("python_perf", "ins", "plug_in", ZstURI.Direction.IN_JACK);

			Showtime.connect_plugs (local_uri_out, remote_uri_in);
			System.Threading.Thread.Sleep (100);

			local_plug_out.fire (12);
		}
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
			yield return new WaitUntil (() => Showtime.plug_event_queue_size () > 0);
			while (Showtime.plug_event_queue_size () > 0) {
				//Pop event and handle it. This is a good place to implement a callback system since we
				//receive the plug handle
				PlugEvent e = Showtime.pop_plug_event ();
				Debug.Log ("Queue got a plug event from: " + e.plug ().get_URI ().name () + " Value: " + showtime_dotnet.convert_to_int_plug (e.plug ()).get_value ());
			}
		}
	}
}
