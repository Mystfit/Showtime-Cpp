import time
import threading
import showtime
from showtime import Showtime as ZST
from showtime import ZstEventCallback, ZstURI, ZstPlugDataEventCallback
from showtime import ZstFilter, ZstComponent, AddFilter


class SinkComponent(ZstComponent):
    def __init__(self, name, parent):
        ZstComponent.__init__(self, "ECHO", name, parent)
        self.plug = self.create_input_plug("in", showtime.ZST_INT)

    def compute(self, plug):
        print("Plug received value {0}".format(plug.value().int_at(0)))


class PushComponent(ZstComponent):
    def __init__(self, name, parent):
        ZstComponent.__init__(self, "OUTPUT", name, parent)
        self.plug = self.create_output_plug("out", showtime.ZST_INT)

    def send(self, val):
        self.plug.value().append_int(val)
        self.plug.fire()


class Watcher(threading.Thread):
    def __init__(self):
        threading.Thread.__init__(self)
        self.setDaemon(True)

    def run(self):
        while True:
            if ZST.event_queue_size() > 0:
                ZST.poll_once()


def test_add():
    # Create entities
    root = ZstComponent("ROOT", "python_test")
    add = AddFilter(root)
    augend_out = PushComponent("augend_out", root)
    addend_out = PushComponent("addend_out", root)
    sum_in = SinkComponent("sum_in", root)
    time.sleep(0.2)

    # Connect cables
    ZST.connect_cable(augend_out.plug.get_URI(), add.augend().get_URI())
    ZST.connect_cable(addend_out.plug.get_URI(), add.addend().get_URI())
    ZST.connect_cable(add.sum().get_URI(), sum_in.plug.get_URI())
    time.sleep(1)
    
    # Fire values
    augend_out.send(1)
    for i in range(10000):
        addend_out.send(i*2)
        #ZST.poll_once()
    time.sleep(1)


if __name__ == "__main__":
    watch = Watcher()
    watch.start()

    ZST.init()
    ZST.join("127.0.0.1")
    
    test_add()
    
    ZST.destroy()
