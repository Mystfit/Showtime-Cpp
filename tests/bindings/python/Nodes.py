import showtime
import showtime.showtime as ZST
import time

class Plugger(ZST.ZstComponent):
	def on_registered(self):
		self.in_plug = ZST.ZstInputPlug("in", ZST.ZstValueType_IntList)
		self.out_plug = ZST.ZstOutputPlug("out", ZST.ZstValueType_IntList)
		self.add_child(self.in_plug)
		self.add_child(self.out_plug)

	def on_compute(self, plug):
		print("Plug {} received value {}".format(plug.URI().path(), plug.int_at(0)))


client = ZST.ShowtimeClient()
client.init("python_nodes", True)
client.auto_join_by_name("stage")

a = Plugger("a")
b = Plugger("b")
client.get_root().add_child(a)
client.get_root().add_child(b)

try:
	while True:
		client.poll_once()
		time.sleep(0.1)
except KeyboardInterrupt:
	print("Received keyboard interrupt")

client.destroy()
