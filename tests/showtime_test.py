import time
import threading
import showtime
from showtime import Showtime as ZST
from showtime import ZstEventCallback, ZstURI, ZstPlugDataEventCallback
from showtime import ZstPatch, ZstFilter, AddFilter


class PlugCallback(ZstPlugDataEventCallback):
    def run(self, plug):
        print("Plug received value {0}".format(plug.value().int_at(0)))

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
ZST.join("127.0.0.1")

# Create entities
root = ZstPatch("python_test")
test_filter = ZstFilter("test_filter", root)
add = AddFilter(root)
plug_callback = PlugCallback()

# Create plugs
plug_augend = test_filter.create_output_plug("p_augend", showtime.ZST_INT)
plug_addend = test_filter.create_output_plug("p_addend", showtime.ZST_INT)
plug_sum = test_filter.create_input_plug("p_sum", showtime.ZST_INT)
plug_sum.input_events().attach_event_callback(plug_callback)
time.sleep(0.2)

# Connect cables
ZST.connect_cable(plug_augend.get_URI(), add.augend().get_URI())
ZST.connect_cable(plug_addend.get_URI(), add.addend().get_URI())
ZST.connect_cable(add.sum().get_URI(), plug_sum.get_URI())
time.sleep(0.2)

# Fire values
plug_augend.value().append_int(27)
plug_addend.value().append_int(3)
plug_augend.fire()
plug_addend.fire()
time.sleep(1)

print("Done")
ZST.destroy()
