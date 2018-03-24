import time
import os
import sys
import threading
import showtime as ZST
import subprocess
from showtime import ZstComponent, ZstActivationEvent


class SinkComponent(ZstComponent):
    def __init__(self, name):
        ZstComponent.__init__(self, name)
        self.plug = self.create_input_plug("in", ZST.ZST_INT)
        self.last_received_value = 0

    def compute(self, plug):
        ZST.app(ZST.notification, "Received message on sink")
        self.last_received_value = plug.int_at(0)


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
            time.sleep(0.001)


if __name__ == "__main__":

    server = None

    # Start server
    if sys.argc > 1:
        server_exe = os.path.abspath(sys.argv[1])
        print("Starting Showtime server from ".format(server_exe))
        server = subprocess.Popen([server_exe, "t"], stdin=subprocess.PIPE, shell=True)
    
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
    ZST.activate_entity(push)
    ZST.activate_entity(sink)

    # Connect cables
    ZST.connect_cable(sink.plug, push.plug)
    time.sleep(0.1)

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
    ZST.deactivate_entity(push)
    ZST.deactivate_entity(sink)
    ZST.destroy()

    if server:
        server.communicate(b"$TERM\n")
        server.wait()
    
    print("Python test finished with status {}".format(status))
    if(status):
        sys.exit(status)
    