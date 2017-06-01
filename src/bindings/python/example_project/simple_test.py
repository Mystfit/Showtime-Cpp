import showtime_py.showtime_py as showtime
stage = showtime.ZstStage_create_stage()

showtime.Showtime_join("127.0.0.1")
showtime.Showtime_create_performer("bob")
plugA = showtime.Showtime_create_int_plug(showtime.ZstURI("bob", "ins", "plugA", showtime.ZstURI.OUT_JACK))
plugB = showtime.Showtime_create_int_plug(showtime.ZstURI("bob", "ins", "plugB", showtime.ZstURI.IN_JACK))

class Callback(showtime.PlugCallback):
    def __init__(self):
        showtime.PlugCallback.__init__(self)

    def run(self, plug):
        print("You're an idiot")

plugB.attach_recv_callback(Callback())

showtime.Showtime_connect_plugs(plugA.get_URI(), plugB.get_URI())
plugA.fire(27)

import time
time.sleep(1)

print("Plug final value: " + str(plugB.get_value()))

showtime.destroy()