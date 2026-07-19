import argparse
import sys
import time
from pathlib import Path

from gateway import Gateway

sys.path.append(str(Path(__file__).resolve().parent.parent))
from util import Colors


def main():
    gateway = Gateway()

    print("Switching all pads to 0xFFFFFF")
    gateway.switch_pad(0, Colors.WHITE)

    time.sleep(1)

    gateway.clear_pads()

    print("Flashing pad...")

    gateway.flash_pads(
        [
            (5, 10, 15, Colors.RED),
            (20, 25, 30, Colors.GREEN),
            (35, 40, 45, Colors.BLUE),
        ]
    )

    time.sleep(5)

    print("Fading pad...")
    gateway.switch_pad(0, Colors.WHITE)
    time.sleep(1)

    gateway.fade_pads(
        [
            (5, 10, Colors.RED),
            (20, 25, Colors.GREEN),
            (35, 40, Colors.BLUE),
        ]
    )

    time.sleep(5)

    gateway.clear_pads()


if __name__ == "__main__":
    main()
