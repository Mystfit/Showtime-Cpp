import showtime.showtime as ZST
import time, math, sys, signal

def sigint_handler(signal, frame):
	global client
	print('KeyboardInterrupt is caught')
	client.destroy()
	sys.exit(0)

class Receiver(ZST.ZstComponent):
	def on_registered(self):
		self.inplug = ZST.ZstInputPlug("in", ZST.ZstValueType_FloatList, True)
		self.add_child(self.inplug)

	def on_compute(self, plug):
		print("Receiver got message")

input("Attach debugger now then press a key")

client = ZST.ShowtimeClient()
client.init("udpdestination", True)
client.join("stun.gorkblorf.com:49152")

receiver = Receiver("receiver")
client.get_root().add_child(receiver)

signal.signal(signal.SIGINT, sigint_handler)

while True:
	time.sleep(0.01)
	client.poll_once()

#raw_input("Press any key to exit")
client.destroy()
