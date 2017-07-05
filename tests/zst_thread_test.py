import time
import showtime
import threading

class EventCallback(showtime.ZstEventCallback):
    def __init__(self):
        showtime.ZstEventCallback.__init__(self)

    def run(self, event):
        print("Received event hit: " + event.get_update_type())


# class Watcher(threading.Thread):
#     def __init__(self):
#         threading.Thread.__init__(self)
#         self.setDaemon(True)
#
#     def run(self):
#         while True:
#             while showtime.Showtime_event_queue_size() > 0:
#                 showtime.Showtime_poll_once()
#             # event = showtime.Showtime_pop_event()
#             # print("Received plug event")
#             # if event.get_update_type() == showtime.ZstEvent.PLUG_HIT:
#             #     print("Received plug hit: {0}".format(event.get_first().to_char()))
#
# watch = Watcher()
# watch.start()

showtime.Showtime_init()

stageCallback = EventCallback()
showtime.Showtime_attach_stage_event_callback(stageCallback)
showtime.Showtime_join("127.0.0.1")
perf = showtime.Showtime_create_performer("python_perf")

local_uri_out = showtime.ZstURI_create("python_perf", "ins", "plug_out", showtime.ZstURI.OUT_JACK)
local_uri_in = showtime.ZstURI_create("python_perf", "ins", "plug_in", showtime.ZstURI.IN_JACK)

plug_out = showtime.Showtime_create_int_plug(local_uri_out)
plug_in = showtime.Showtime_create_int_plug(local_uri_in)
showtime.Showtime_connect_cable(local_uri_in, local_uri_out)

time.sleep(0.2)
plug_out.fire(27)
time.sleep(0.2)

showtime.Showtime_poll_once()
time.sleep(0.2)
showtime.Showtime_remove_stage_event_callback(stageCallback)

print("Done")
showtime.Showtime_destroy()

# Listen test
# choice = str(input("Listen for dotnet? y/n"))
# if choice == 'y':
#     try:
#         while True:
#             time.sleep(1)
#     except KeyboardInterrupt:
#         print("Aborting...")
#
#
# # Remote Send test
# choice = input("Send to dotnet? y/n")
# if choice == "y":
#     remote_uri = showtime.ZstURI_create("unity_performer", "ins", "plug_in", showtime.ZstURI.IN_JACK)
#     showtime.Showtime_connect_plugs(remote_uri, local_uri_out)
#     time.sleep(0.1)
#     plug_out.fire(27)
#

