import unittest
import time
import os
import sys
import threading
import subprocess
import showtime.showtime as ZST
from showtime.showtime import ZstComponent, ZstOutputPlug, ZstInputPlug, ShowtimeClient, ShowtimeServer


class SinkComponent(ZstComponent):
    def __init__(self, name):
        ZstComponent.__init__(self, name)
        self.last_received_value = 0
        self.cv = threading.Condition()

    def on_registered(self):
        self.plug = ZstInputPlug("in", ZST.ZstValueType_IntList)
        self.add_child(self.plug)

    def compute(self, plug):
        self.last_received_value = plug.int_at(0)
        ZST.app(ZST.notification, "Sink received value: {}".format(self.last_received_value))
        self.cv.acquire()
        self.cv.notify()
        self.cv.release()


class PushComponent(ZstComponent):
    def __init__(self, name):
        ZstComponent.__init__(self, name)

    def on_registered(self):
        self.plug = ZstOutputPlug("out", ZST.ZstValueType_IntList)
        self.add_child(self.plug)

    def send(self, val):
        self.plug.append_int(val)
        self.plug.fire()


class EventLoop(threading.Thread):
    def __init__(self, client):
        threading.Thread.__init__(self)
        self.client = client
        self.setDaemon(True)
        self.is_running = True

    def stop(self):
        self.is_running = False

    def run(self):
        while self.is_running:
            self.client.poll_once()
            time.sleep(0.001)


class Test_PythonExtensions(unittest.TestCase):
    def setUp(self):
        # Client/server
        server_name = "python_server"
        self.server = ShowtimeServer(server_name)
        self.client = ShowtimeClient()
        self.client.init("test_python", True)

        # Set up event loop
        self.event_loop = EventLoop(self.client)
        self.event_loop.start()

        # Join server
        self.client.auto_join_by_name(server_name)

    def tearDown(self):
        self.event_loop.stop()
        self.client.destroy()
        self.server.destroy()

    def test_get_children(self):
        component = PushComponent("component")
        self.client.get_root().add_child(component)
        self.assertEqual(len(component.get_child_entities()), 1)
        self.assertEqual(len(component.get_child_entities(include_parent=True)), 2)
        self.assertEqual(len(self.client.get_root().get_child_entities(recursive=True)), 2)

    def test_send_graph_message(self):
        push = PushComponent("push")
        sink = SinkComponent("sink")

        # Activate entities
        self.client.get_root().add_child(push)
        self.client.get_root().add_child(sink)

        # Connect cables
        cable = self.client.connect_cable(sink.plug, push.plug)
        self.assertIsNotNone(cable)
        
        # Send values
        sending_val = 42
        push.send(42)

        # Wait for value
        sink.cv.acquire()
        sink.cv.wait(5.0)
        sink.cv.release()

        # Check value
        self.assertEqual(sink.last_received_value, sending_val)


if __name__ == '__main__':
    unittest.main()