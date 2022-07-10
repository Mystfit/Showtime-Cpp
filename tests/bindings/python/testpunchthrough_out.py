import showtime.showtime as ZST
import time, math, sys

input("Attach debugger now then press a key")
target = ZST.ZstURI("udpdestination/receiver/in")

client = ZST.ShowtimeClient()
client.init("udp_remote_test", True)

out = None
sender = None

def on_sync(client, server):
	global out, sender
	sender = ZST.ZstComponent("sender")
	client.get_root().add_child(sender)
	out = ZST.ZstOutputPlug("out", ZST.ZstValueType_FloatList, False)
	sender.add_child(out)

	# assert out.can_fire()
	in_ent = client.find_entity(target)
	input_plug = ZST.cast_to_input_plug(in_ent)
	if not client.connect_cable(input_plug, out):
		sys.exit()

	
client.connection_events().synchronised_graph.add(on_sync)
client.join_async("stun.gorkblorf.com:49152")

count = 0.0
try:
	for i in range(5001):
		if out:
			out.append_float(math.sin(count) * 0.5 + 0.5)
			out.fire()
			count += 0.01
		time.sleep(0.01)
		client.poll_once()

except KeyboardInterrupt:
	pass

#raw_input("Press any key to exit")
client.destroy()