"""

mixedmode 
simple midi router

"""

import click
import mido
import signal
import sys
from functools import partial
from threading import Event


DEFAULT_INPUT = "Launchpad Pro MK3 LPProMK3 MIDI"
DEFAULT_OUTPUT = "Teensy MIDI"


def open_ports(input_name, output_name):
    """
    Open the first matching input and output port
    """

    for name in mido.get_input_names():
        if input_name in name:
            input_port = mido.open_input(name)
            break

    for name in mido.get_output_names():
        if output_name in name:
            output_port = mido.open_output(name)
            break

    return input_port, output_port

    
def route(port, input_name, msg):
    """
    route all incoming messages to port
    """
    if msg.type != "clock":
        print(f"{input_name} -> {port.name} : {msg}")
    port.send(msg)


@click.command()
@click.option("-i", "--input", type=click.Choice(mido.get_input_names(), case_sensitive=True), help="Name of input port", default=DEFAULT_INPUT)
@click.option("-o", "--output", type=click.Choice(mido.get_output_names(), case_sensitive=True), help="Name of output port", default=DEFAULT_OUTPUT)
def main(input, output):
    input_port, output_port = open_ports(input, output)
    input_port.callback = partial(route, output_port, input_port.name)

    print(f"Now routing from {input_port.name} to {output_port.name}")
    
    signal.signal(signal.SIGINT, signal.SIG_DFL) #allows to kill Event() thread
    Event().wait() # wait forever


if __name__ == "__main__":
    sys.exit(main())
