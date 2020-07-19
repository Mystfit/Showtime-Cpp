from __future__ import print_function
import unittest

try:
    from unittest.mock import Mock
except ImportError:
    from mock import Mock

import time
import os
import sys
import threading
import subprocess
import showtime.showtime as ZST
from showtime.showtime import ZstURI, ZstComponent, ZstOutputPlug, ZstInputPlug, ShowtimeClient, ShowtimeServer, ZstServerAddress

server_name = "python_server"

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
        self.server = ShowtimeServer()
        self.server.init(server_name)
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

    def test_entity_callbacks(self):
        # Register event callbacks
        registered_handler = Mock()
        entity = ZstComponent("test_entity")
        entity.entity_events().entity_registered.add(registered_handler)

        # Make sure we receive a registered entity event
        self.client.get_root().add_child(entity)
        self.assertTrue(registered_handler.called)

    def test_connected_callbacks(self):
        # Register event callbacks
        connected = Mock()
        disconnected = Mock()
        self.client.leave()
        self.client.connection_events().connected_to_server.add(connected)
        self.client.connection_events().disconnected_from_server.add(disconnected)

        # Join server
        self.client.auto_join_by_name(server_name)
        
        # Join server
        self.client.auto_join_by_name(server_name)
        self.assertTrue(connected.called)

        # Leave server
        self.client.leave()
        self.assertTrue(disconnected.called)


class Test_Client(unittest.TestCase):
    def setUp(self):
        self.client = ShowtimeClient()
        self.client.init("test_python", True)

    def tearDown(self):
        self.client.destroy()

    def test_server_discovery_callbacks(self):
        # Register event callbacks
        discovered_handler = Mock()
        lost_handler = Mock()
        self.client.connection_events().server_discovered.add(discovered_handler)
        self.client.connection_events().server_lost.add(lost_handler)
        
        # Start a second server to emit a new beacon
        server = ZST.ShowtimeServer()
        server.init("discovery_server")
        
        # Make sure we received the beacon
        time.sleep(1)
        self.client.poll_once()
        self.assertTrue(discovered_handler.called)

        # Check that we receive a timeout event
        server.destroy()
        time.sleep(6)
        self.client.poll_once()
        self.assertTrue(lost_handler.called)


class Test_PythonCallbacks(unittest.TestCase):
    def setUp(self):
        # Client/server
        self.server = ShowtimeServer()
        self.server.init(server_name)
        self.client = ShowtimeClient()
        self.client.init("test_python", True)
        self.client.auto_join_by_name(server_name)

    def tearDown(self):
        self.client.destroy()
        self.server.destroy()

    def test_synchronisable_callbacks(self):
        # Register event handlers
        activated = Mock()
        updated = Mock()
        deactivated = Mock()
        destroyed = Mock()

        # Create components and register callbacks
        component = ZstComponent("child")
        component.synchronisable_events().synchronisable_activated.add(activated)
        component.synchronisable_events().synchronisable_deactivated.add(deactivated)
        component.synchronisable_events().synchronisable_destroyed.add(destroyed)

        # Add child to trigger callback
        self.client.get_root().add_child(component)
        self.assertTrue(activated.called)

        # Deactive entity
        self.client.deactivate_entity(component)
        self.assertTrue(deactivated.called)

        # Destroy entity
        del component
        self.assertTrue(destroyed.called)


class Test_DualClientEvents(unittest.TestCase):
    def setUp(self):
        # Client/server
        self.server_name = "DualClientEvents"
        self.server = ShowtimeServer()
        self.server.init(server_name)
        self.client = ShowtimeClient()
        self.client.init("test_python", True)

        # Create a second (remote) client
        self.remote_client = ShowtimeClient()
        self.remote_client.init("test_python2", True)

        # Join server
        self.client.auto_join_by_name(server_name)
        self.remote_client.auto_join_by_name(server_name)

    def tearDown(self):
        self.remote_client.destroy()
        self.client.destroy()
        self.server.destroy()
    
    def test_performer_hierarchy_callbacks(self):
        # Register event callbacks
        arrived = Mock()
        left = Mock()
        self.client.hierarchy_events().performer_arriving.add(arrived)
        self.client.hierarchy_events().performer_leaving.add(left)
        
        # Make sure we received an arrived event
        self.client.poll_once()
        self.assertTrue(arrived.called)

        # Make sure we receive a leave event
        self.remote_client.leave()
        time.sleep(0.1)
        self.client.poll_once()
        self.assertTrue(left.called)

    def test_entity_hierarchy_callbacks(self):
        # Register event callbacks
        arrived = Mock()
        left = Mock()
        self.client.hierarchy_events().entity_arriving.add(arrived)
        self.client.hierarchy_events().entity_leaving.add(left)
        
        # Create component
        component = ZST.ZstComponent("child")
        self.remote_client.get_root().add_child(component)
        self.assertTrue(component.is_registered())

        # Wait for entity arrived event
        time.sleep(0.1)
        self.client.poll_once()
        self.assertTrue(arrived.called)
        #self.assertTrue(ZST.ZstURI.equals(component.URI(), ZstURI("test_python/child")))

        # Remove component
        del component

        # Wait for entity left event
        time.sleep(0.5)
        self.client.poll_once()
        self.assertTrue(left.called)
        time.sleep(0.5)


    def test_entity_renamed(self):
        # Register event callbacks
        client_updated = Mock()
        self.client.hierarchy_events().entity_updated.add(client_updated)
        
        # Create component
        component = ZST.ZstComponent("child")
        self.remote_client.get_root().add_child(component)

        # Find local proxy entity to watch for update events
        time.sleep(0.1)
        self.client.poll_once()
        proxy_component = self.client.find_entity(component.URI())

        # Rename action triggers updates
        component.set_name("renamed_child")

        # Wait for entity updated event
        time.sleep(0.1)
        self.client.poll_once()
        self.assertTrue(client_updated.called)
        self.assertTrue(ZstURI.equal(proxy_component.URI(), component.URI()))
    
    def test_factory_hierarchy_callbacks(self):
        # Register event callbacks
        arrived = Mock()
        left = Mock()
        self.client.hierarchy_events().factory_arriving.add(arrived)
        self.client.hierarchy_events().factory_leaving.add(left)
        
        # Create factory
        factory = ZST.ZstEntityFactory("factory")
        self.remote_client.register_factory(factory)

        # Wait for entity arrived event
        time.sleep(0.1)
        self.client.poll_once()
        self.assertTrue(arrived.called)

        # Remove component
        del factory

        # Wait for entity left event
        time.sleep(0.1)
        self.client.poll_once()
        self.assertTrue(left.called)

    def test_cable_session_events(self):
        # Register event callbacks
        created = Mock()
        destroyed = Mock()
        self.client.session_events().cable_created.add(created)
        self.client.session_events().cable_destroyed.add(destroyed)

        # Create components
        push = PushComponent("push")
        sink = SinkComponent("sink")
        self.client.get_root().add_child(sink)
        self.remote_client.get_root().add_child(push)
        time.sleep(0.1)
        proxy_push_plug = ZST.cast_to_output_plug(self.client.find_entity(push.plug.URI()))

        # Create cable
        cable = self.client.connect_cable(sink.plug, proxy_push_plug)
        
        # Wait for cable arrived event
        time.sleep(0.1)
        self.client.poll_once()
        self.assertTrue(created.called)

        # Destroy cable
        self.client.destroy_cable(cable)

        # Wait for cable destroyed event
        time.sleep(0.1)
        self.client.poll_once()
        self.assertTrue(destroyed.called)
        

if __name__ == '__main__':
    unittest.main()
