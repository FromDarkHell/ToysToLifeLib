import array
import platform
import sys
import time
from pathlib import Path
from typing import Dict, List, Literal, Optional, Tuple, Union
import usb.core

sys.path.append(str(Path(__file__).resolve().parent.parent))
from util import Color, Colors

USB_IDS: Dict[str, Tuple[int, int]] = {
    "xbox_360": (0x24C6, 0xFA00),
    "ps3": (0x0E6F, 0x0129),
}


class Portal:
    """Represents a Disney Infinity portal peripheral"""

    MAX_LENGTH = 0x20

    def __init__(
        self, platform: Union[Literal["xbox_360"], Literal["ps3"]], verbose: bool = True
    ):
        self.verbose = verbose
        self.used_kernel_driver = False
        self._cid = 1

        self.VENDOR_ID, self.PRODUCT_ID = USB_IDS[platform]
        self.platform = platform

        # Initialise USB connection to the device
        self.dev = self._init_usb()

    def __del__(self):
        if self.used_kernel_driver:
            self.dev.attach_kernel_driver(0)  # type: ignore

    def _bytes_to_hex(self, bytes) -> str:
        return " ".join(format(x, "02X") for x in bytes)

    def get_cid(self):
        self._cid += 1
        return self._cid

    def _init_usb(self):
        """
        Connect to and initialise the portal
        """
        # Let's try and find the USB Device

        dev: usb.core.Device = usb.core.find(idVendor=self.VENDOR_ID, idProduct=self.PRODUCT_ID)  # type: ignore

        # Double check that the device was found
        if dev is None:
            raise ValueError("Device not found")

        if platform.system() == "Linux":
            if dev.is_kernel_driver_active(0):
                dev.detach_kernel_driver(0)
                self.used_kernel_driver = True

        # Initialise portal
        if self.verbose:
            print(f"Found portal at port #{dev.port_number}")

        # Set the active configuration. With no arguments, the first
        # configuration will be the active one
        dev.set_configuration()
        self.dev = dev

        print(f"{self.dev}")

        self.wake()

        return dev

    def _send_command(self, command: List[int]):
        # One byte must be left unfilled in order to fill the checksum
        assert len(command) <= 31

        def convert_to_packet(command: List[int]):
            if command[0] != 0xFF:
                command.insert(0, 0xFF)
                command.insert(1, len(command) - 1)

            checksum = 0
            for word in command:
                checksum += word
                if checksum >= 256:
                    checksum -= 256

            message = [*command, checksum]

            if self.platform == "xbox_360":
                message = [0x0B, 0x10, *message]

            assert len(message) <= 32
            while len(message) < 32:
                message.append(0x00)

            return message

        packet = convert_to_packet(command)

        print(f"Sending packet: {self._bytes_to_hex(packet)}")
        self.dev.write(2, packet)

        try:
            response = self.dev.read(0x81, Portal.MAX_LENGTH, timeout=1_000)
            print(f"Received response: {self._bytes_to_hex(response)}")
        except usb.core.USBError as e:
            if e.args != ("Operation timed out",):
                raise

    def connected(self) -> bool:
        dev: usb.core.Device = usb.core.find(idVendor=self.VENDOR_ID, idProduct=self.PRODUCT_ID)  # type: ignore
        if dev == None:
            return False

        return dev.address == self.dev.address

    def wake(self):
        # Startup
        _ENCODED = "(c) Disney 2013".encode("ascii")
        self._send_command([0x80, self.get_cid(), *_ENCODED])
        # response = self.dev.read(0x81, Portal.MAX_LENGTH, timeout=1_000)
        # print(f"Received init response: {self._bytes_to_hex(response)}")

    def switch_pad(self, pad_id: int, color: Color):
        """
        Changes the color of one (or all) pads immediately
        pad_id: 0 = All, 1 = Center, 2 = Left, 3 = Right
        Color values are clamped betwen 0 and 255, in RGB order
        """

        command = [0x90, self.get_cid(), pad_id, color.r, color.g, color.b]
        self._send_command(command)

    def flash_pad(
        self,
        pad_id: int,
        on_length: int,
        off_length: int,
        pulse_count: int,
        color: Color,
    ):
        command = [
            0x93,
            self.get_cid(),
            pad_id,
            on_length,
            off_length,
            pulse_count,
            color.r,
            color.g,
            color.b,
        ]

        self._send_command(command)

    def flash_pads(self, pad_data: List[Optional[Tuple[int, int, int, Color]]]):
        """
        Flashes all pads with their own individual colors and rates
        Each pad is represented by a tuple in the format:
            - `(on_length, off_length, pulse_count, (R, G, B))`
        Pads are in the order of: (center, left, right)
        Color values must be between 0 and 255 (0x00 - 0xFF).
        Use `None` to ignore flashing that specific pad. Ignored pads continue to do whatever they were doing previously.

        - `on_length`: A value of 0x00 is practically non-existant, and a value of 0xFF is ~25.5 seconds
        - `off_length`: A value of 0x00 is practically non-existant, and a value of 0xFF is ~25.5 seconds
        - `pulse_count`: An odd value leaves pad in new colour, even leaves pad in old, except for 0x00, which does nothing. Values above 0x96 dont stop.
        """
        assert len(pad_data) == 3
        command = [0x97, self.get_cid()]
        for pad in pad_data:
            enable, on, off, pulse = 0, 0, 0, 0
            color = Color(0, 0, 0)
            if pad != None:
                enable = 1
                on, off, pulse, color = pad
            command += [enable, on, off, pulse, color.r, color.g, color.b]

        self._send_command(command)

    def fade_pad(self, pad_id: int, speed: int, count: int, color: Color):
        command = [
            0x92,
            self.get_cid(),
            pad_id,
            speed,
            count,
            color.r,
            color.g,
            color.b,
        ]

        self._send_command(command)

    def fade_pads(self, pad_data: List[Optional[Tuple[int, int, Color]]]):
        assert len(pad_data) == 3
        command = [0x96, self.get_cid()]
        for pad in pad_data:
            enable, fade, count = 0, 0, 0
            color = Color(0, 0, 0)

            if pad != None:
                enable = 1
                fade, count, color = pad

            command += [enable, fade, count, color.r, color.g, color.b]

        self._send_command(command)


if __name__ == "__main__":
    px = Portal("xbox_360")
    time.sleep(0.5)

    px.switch_pad(0, Colors.BLACK)

    px.switch_pad(1, Colors.RED)
    px.switch_pad(2, Colors.GREEN)
    px.switch_pad(3, Colors.BLUE)

    # px.flash_pad(2, 20, 20, 128, Colors.RED)

    while True:
        time.sleep(0.1)
