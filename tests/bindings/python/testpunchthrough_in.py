import showtime.showtime as ZST
import time, math, sys, signal

def sigint_handler(signal, frame):
	global client
	print('KeyboardInterrupt is caught')
	client.destroy()
	sys.exit(0)

class Receiver(ZST.ZstComputeComponent):
	def on_registered(self):
		self.inplug = ZST.ZstInputPlug("in", ZST.ZstValueType_FloatList, -1, True)
		self.add_child(self.inplug)
		self.num_hits = 0

	def compute(self, plug):
		print("Receiver got value {0}".format(self.inplug.float_at(0)))
		self.num_hits += 1

input("Attach debugger now then press a key")

client = ZST.ShowtimeClient()
client.init("udpdestination", True)
client.join("stun.gorkblorf.com:49152")

receiver = Receiver("receiver")
client.get_root().add_child(receiver)

signal.signal(signal.SIGINT, sigint_handler)

starttime = time.time()
while receiver.num_hits <= 5000 and time.time() - starttime < 10.0
	time.sleep(0.01)
	client.poll_once()

print("Number of messages received: {}".format(receiver.num_hits))

#raw_input("Press any key to exit")
client.destroy()
