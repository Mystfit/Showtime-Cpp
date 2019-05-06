import time
import os
import sys
import threading
import subprocess
import showtime.showtime as ZST
from showtime.showtime import ZstComponent


class SinkComponent(ZstComponent):
    def __init__(self, name):
        ZstComponent.__init__(self, name)
        self.plug = self.create_input_plug("in", ZST.ZST_INT)
        self.last_received_value = 0

    def compute(self, plug):
        self.last_received_value = plug.int_at(0)
        ZST.app(ZST.notification, "Sink received value: {}".format(self.last_received_value))


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
        self.is_running = True

    def stop(self):
        self.is_running = False

    def run(self):
        while self.is_running:
            ZST.poll_once()
            time.sleep(0.001)


if __name__ == "__main__":    
    server = ZST.create_server("python_server", ZST.STAGE_ROUTER_PORT);
    
    # Start client
    ZST.init("python_test", True)
    ZST.join("127.0.0.1")
    
    # Set up event loop
    event_loop = EventLoop()
    event_loop.start()

    # Create components
    push = PushComponent("push")
    sink = SinkComponent("sink")

    # Activate entities
    ZST.get_root().add_child(push)
    ZST.get_root().add_child(sink)

    # Connect cables
    ZST.connect_cable(sink.plug, push.plug)
    
    # Send values
    sending_val = 42
    push.send(42)

    # Wait until sink receives value
    max_loops = 100
    loops = 0
    while sink.last_received_value != sending_val and loops < max_loops:
        time.sleep(0.01)
        loops += 1
    status = 0 if sink.last_received_value == sending_val else 1
    print("Looped {} times. Last received value: {}".format(loops, sink.last_received_value))

    # Cleanup
    event_loop.stop()
    ZST.destroy_server(server)
    ZST.destroy()
    
    print("Python test finished with status {}".format(status))
    if(status):
        sys.exit(status)
    