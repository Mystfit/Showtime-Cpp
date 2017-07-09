import time
import threading
from showtime import Showtime as ZST
from showtime import ZstEventCallback, ZstURI


class EventCallback(ZstEventCallback):
    def __init__(self):
        ZstEventCallback.__init__(self)

    def run(self, event):
        print("Received stage event {0} from {1}".format(
            event.get_update_type(), event.get_first().to_char()))


class Watcher(threading.Thread):
    def __init__(self):
        threading.Thread.__init__(self)
        self.setDaemon(True)

    def run(self):
        while True:
            if ZST.event_queue_size() > 0:
                ZST.poll_once()

watch = Watcher()
watch.start()

ZST.init()

stageCallback = EventCallback()
ZST.attach_stage_event_callback(stageCallback)
ZST.join("127.0.0.1")
perf = ZST.create_performer("python_perf")

uri_out = ZstURI.create("python_perf", "ins", "plug_out", ZstURI.OUT_JACK)
uri_in = ZstURI.create("python_perf", "ins", "plug_in", ZstURI.IN_JACK)

plug_out = ZST.create_int_plug(uri_out)
plug_in = ZST.create_int_plug(uri_in)
ZST.connect_cable(uri_in, uri_out)

time.sleep(0.2)
plug_out.fire(27)
time.sleep(0.2)

print("Done")
ZST.remove_stage_event_callback(stageCallback)
ZST.destroy()
