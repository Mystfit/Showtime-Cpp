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
        self.last_received_value = 0

    def compute(self, plug):
        self.last_received_value = plug.value().int_at(0)
        if self.last_received_value % 1000 == 0:
            print("Plug received value {0}".format(self.last_received_value))


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
    num_loops = 1000
    for i in range(num_loops):
        for j in range(num_loops):
            addend_out.send(i * 1000 + j)
        print("Finished loop" + str(i))
        time.sleep(0.01)
        #ZST.poll_once() 
        #time.sleep(0.001)

    while sum_in.last_received_value < num_loops * num_loops:
        time.sleep(0.5)

    print("Ran loop {0} times. Final sum: {1}".format(num_loops, sum_in.last_received_value))


if __name__ == "__main__":
    watch = Watcher()
    watch.start()

    ZST.init()
    ZST.join("127.0.0.1")

    raw_input("Press any key to continue...")
    
    test_add()
    
    ZST.destroy()
