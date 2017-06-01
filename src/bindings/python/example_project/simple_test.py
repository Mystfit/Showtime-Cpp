import time
import showtime_py.showtime_py as showtime

tage = showtime.ZstStage_create_stage()

showtime.Showtime_join("127.0.0.1")
showtime.Showtime_create_performer("bob")
plugA = showtime.Showtime_create_int_plug(showtime.ZstURI("bob", "ins", "plugA", showtime.ZstURI.OUT_JACK))
plugB = showtime.Showtime_create_int_plug(showtime.ZstURI("bob", "ins", "plugB", showtime.ZstURI.IN_JACK))

class Callback(showtime.PlugCallback):
    def __init__(self):
        showtime.PlugCallback.__init__(self)

    def run(self, plug):
        print("You're an idiot")
        plugB.remove_recv_callback(self)

plugB.attach_recv_callback(Callback().__disown__())
showtime.Showtime_connect_plugs(plugA.get_URI(), plugB.get_URI())

time.sleep(0.1)
plugA.fire(27)
time.sleep(0.1)

print("Plug final value: " + str(plugB.get_value()))

showtime.Showtime_destroy()