import showtime
import showtime.showtime as ZST
import time

client = ZST.ShowtimeClient()
client.init("python_nodes", True)
client.auto_join_by_name("stage")

a = ZST.ZstComponent("a_comp")
client.get_root().add_child(a)
plug1 = ZST.ZstInputPlug("plug1", ZST.ZstValueType_IntList)
plug2 = ZST.ZstInputPlug("plug2", ZST.ZstValueType_IntList)
a.add_child(plug1)
a.add_child(plug2)

b = ZST.ZstComponent("b_comp")
a.add_child(b)
plug3 = ZST.ZstInputPlug("plug3", ZST.ZstValueType_IntList)
plug4 = ZST.ZstInputPlug("plug4", ZST.ZstValueType_IntList)
b.add_child(plug3)
b.add_child(plug4)


try:
	while True:
		client.poll_once()
		time.sleep(0.1)
except KeyboardInterrupt:
	pass
