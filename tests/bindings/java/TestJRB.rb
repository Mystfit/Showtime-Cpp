require 'java'
require 'ShowtimeJava'

# Import showtime objects
java_import "io.showtime.ShowtimeJava"
java_import "io.showtime.LogLevel"
java_import "io.showtime.ZstURI"
java_import "io.showtime.ZstComponent"
java_import "io.showtime.ZstInputPlug"
java_import "io.showtime.ZstOutputPlug"
java_import "io.showtime.ZstValueType"


#
# A component is an entity that implements a compute function and can contain plugs.
# 
class TestOutputComponent < ZstComponent
    attr_reader :output

    # Component constructor
    def initialize(name)
        # We need to init this component with a name. This will become part of the 
        # URI of this component and all child entities
        super(name)

        # Create an output plug that will send integers
        @output = self.create_output_plug("out", ZstValueType::ZST_INT)
    end

    def send(num)
        # Add the number to the plug value array
        @output.append_int(num)

        # Send all values within this plug to all other connected plugs. 
        # The plug will be cleared once fired
        @output.fire
    end
end


#
# A component is an entity that implements a compute function and can contain plugs.
# 
class TestInputComponent < ZstComponent
    attr_reader :input

    def initialize(name)
        super(name)

        # An input plug will receive values from other plugs that are connected via a cable
        @input = self.create_input_plug("in", ZstValueType::ZST_INT)
    end

    # The compute fn will run whenever a plug attached to this component receives a value.
    # The argument receiving_plug will be the proxy plug object that received the value
    def compute(receiving_plug)
        uri  = receiving_plug.URI()
        val_index = 0
        while val_index < receiving_plug.size()
            value = receiving_plug.int_at(val_index)
            val_index += 1
            ShowtimeJava.app(LogLevel::notification, "#{uri} received #{value}")
        end
    end
end


def test_graph
    # Create entities
    input_component = TestInputComponent.new("jrb_in")
    output_component = TestOutputComponent.new("jrb_out")

    # All entities need to be activated on the stage server before they will show up in the performance graph
    # Entities will be automatically parented to the root performer if they have no parent.
    ShowtimeJava.activate_entity(input_component)
    ShowtimeJava.activate_entity(output_component)

    # We can get entities from the graph if we know their URI
    puts "Component cached in graph with URI: #{ShowtimeJava.find_entity(input_component.URI()).URI()}"
    puts "Component cached in graph with URI: #{ShowtimeJava.find_entity(output_component.URI()).URI()}"
    puts "Plug cached in graph with URI: #{ShowtimeJava.find_entity(output_component.output.URI()).URI()}"

    # Connect a cable between the input and output plugs of our two components
    cable = ShowtimeJava.connect_cable(input_component.input, output_component.output)

    # Tell the output component to send a value - this should print 42 from the input component
    output_component.send(42)
    
    # Since we're sending a value over the network, briefly wait so that we don't exit before 
    # the event loop can process the incoming message
    sleep(0.001)

end

# Initialise the library with the name of our performer - the performer is the root
# of all of the entities that we own locally.
ShowtimeJava.init("jrbtester", true)

# Create an event loop thread to poll for incoming events (async calls, plug values etc)
event_loop = Thread.new do
    loop do 
        ShowtimeJava.poll_once()
    end
end

# Join the stage server
ShowtimeJava.join("127.0.0.1")

# Run graph test
test_graph

# Exit event loop
event_loop.exit
event_loop.join

# You MUST clean up the library on exit (Windows) otherwise risk ZMQ hanging
ShowtimeJava.destroy()
