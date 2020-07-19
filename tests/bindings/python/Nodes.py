import showtime
import showtime.showtime as ZST
import time

client = ZST.ShowtimeClient()
client.init("python_nodes", True)
client.auto_join_by_name("stage")

a = ZST.ZstComponent("a")
client.get_root().add_child(a)

b = ZST.ZstComponent("b")
a.add_child(b)

try:
	while True:
		client.poll_once()
		time.sleep(0.1)
except KeyboardInterrupt:
	pass
