import time
import threading
import showtime as ZST
from showtime import ZstComponent, AddFilter, ZstActivationEvent


class SinkComponent(ZstComponent):
    def __init__(self, name):
        ZstComponent.__init__(self, name)
        self.plug = self.create_input_plug("in", ZST.ZST_INT)
        self.last_received_value = 0

    def compute(self, plug):
        self.last_received_value = plug.int_at(0)
        # if self.last_received_value % 1000 == 0:
        print("Plug received value {0}".format(self.last_received_value))


class PushComponent(ZstComponent):
    def __init__(self, name):
        ZstComponent.__init__(self, name)
        self.plug = self.create_output_plug("out", ZST.ZST_INT)

    def send(self, val):
        self.plug.append_int(val)
        self.plug.fire()


class EventLoop(threading.Thread):
    def __init__(self):
        threading.Thread.__init__(self)
        self.setDaemon(True)

    def run(self):
        while True:
            ZST.poll_once()


def test_add():
    # Create entities
    add = AddFilter("adder")
    augend = PushComponent("aug_out")
    addend = PushComponent("add_out")
    sum_in = SinkComponent("sum_in")

    # Activate entities
    ZST.activate_entity(add)
    ZST.activate_entity(augend)
    ZST.activate_entity(addend)
    ZST.activate_entity(sum_in)

    # Connect cables
    ZST.connect_cable(add.augend(), augend.plug)
    ZST.connect_cable(add.addend(), addend.plug)
    ZST.connect_cable(sum_in.plug, add.sum())

    # Fire values
    augend.send(1)
    num_loops = 10000
    for i in range(num_loops):
        augend.send(i)
        addend.send(1)
        ZST.poll_once()

    while sum_in.last_received_value < num_loops:
        ZST.poll_once()

    print("Ran loop {0} times. Final sum: {1}".format(num_loops, sum_in.last_received_value))

    # Cleanup
    ZST.deactivate_entity(add)
    ZST.deactivate_entity(addend)
    ZST.deactivate_entity(augend)
    ZST.deactivate_entity(sum_in)

if __name__ == "__main__":
    event_loop = EventLoop()
    # event_loop.start()

    ZST.init("python_test", True)
    ZST.join("127.0.0.1")

    test_add()
    ZST.destroy()
