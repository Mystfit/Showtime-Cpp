import time
import showtime
import threading

class PlugHitCallback(showtime.ZstEventCallback):
    def __init__(self):
        showtime.ZstEventCallback.__init__(self)

    def run(self, event):
        print("Received plug hit")


class Watcher(threading.Thread):
    def __init__(self):
        threading.Thread.__init__(self)
        self.setDaemon(True)

    def run(self):
        while True:
            event = showtime.Showtime_pop_event()
            print("Received plug event")
            if event.get_update_type() == showtime.ZstEvent.PLUG_HIT:
                print("Received plug hit: {0}".format(event.get_first().to_char()))

watch = Watcher()
watch.start()

showtime.Showtime_init()
showtime.Showtime_join("127.0.0.1")
perf = showtime.Showtime_create_performer("python_perf")

local_uri_out = showtime.ZstURI_create("python_perf", "ins", "plug_out", showtime.ZstURI.OUT_JACK)
local_uri_in = showtime.ZstURI_create("python_perf", "ins", "plug_in", showtime.ZstURI.IN_JACK)

plug_out = showtime.Showtime_create_int_plug(local_uri_out)
plug_in = showtime.Showtime_create_int_plug(local_uri_in)

callback = PlugHitCallback()
plug_in.attach_recv_callback(callback)

print(plug_in.get_URI().to_char())

showtime.Showtime_connect_cable(local_uri_in, local_uri_out)
time.sleep(0.1)
plug_out.fire(27)
time.sleep(0.1)

# Listen test
choice = str(input("Listen for dotnet? y/n"))
if choice == 'y':
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("Aborting...")


# Remote Send test
choice = input("Send to dotnet? y/n")
if choice == "y":
    remote_uri = showtime.ZstURI_create("unity_performer", "ins", "plug_in", showtime.ZstURI.IN_JACK)
    showtime.Showtime_connect_plugs(remote_uri, local_uri_out)
    time.sleep(0.1)
    plug_out.fire(27)

print("Done")
showtime.Showtime_destroy();
