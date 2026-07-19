import argparse
import sys
import threading
import time
from pathlib import Path

from gateway import Gateway

sys.path.append(str(Path(__file__).resolve().parent.parent))
from util import Color, Colors


def main():
    gateway = Gateway(platform="xbox_360")

    if True:
        gateway.switch_pad(0, Colors.MAGENTA)
    elif False:
        gateway.flash_pads(
            [
                (5, 2, 10, Colors.RED),
                (5, 2, 15, Colors.GREEN),  # Left Pad = Stays Green
                (5, 2, 20, Colors.BLUE),
            ]
        )
    elif False:
        gateway.flash_pad(0, 5, 2, 10, Colors.GREEN)
    elif False:
        # Structure: PAD ID, speed, count, (red, green, blue)

        # This example fades the pad up from black -> red -> black in 5s (the full cycle takes 5s)
        gateway.fade_pad(0, 50, 2, Colors.RED)

        # This example fades the pad from black -> green -> black -> green -> black all over 10s
        gateway.fade_pad(0, 50, 4, Colors.GREEN)
    elif False:
        gateway.fade_pads(
            [
                (0, 1, Color(76, 32, 0)),  # Center Pad = Red
                (0, 1, Colors.YELLOW),  # Left Pad = Green
                (0, 1, Colors.WHITE),  # Right Pad = Stays Blue
            ]
        )
    elif False:
        gateway.fade_pads(
            [
                (0, 1, Color(0, 0, 24)),  # Center Pad = Blue
                None,
                None,
            ]
        )

        time.sleep(5)

        gateway.fade_pads(
            [
                (0, 1, Colors.BLUE),  # Center Pad = Blue
                None,
                None,
            ]
        )

    # gateway.sniff()


if __name__ == "__main__":
    main()
