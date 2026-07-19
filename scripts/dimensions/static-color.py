import argparse
import sys
import time
import traceback
from pathlib import Path

from gateway import Gateway

sys.path.append(str(Path(__file__).resolve().parent.parent))
from util import Color


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--color", type=str, default="#FFFFFF")
    parser.add_argument("--delay", type=float, default=5)

    args = parser.parse_args()

    color = Color.from_hex(args.color)

    while True:
        try:
            gateway = Gateway(platform="xbox_360", verbose=True)

            gateway.switch_pad(0, color)

            while gateway.connected():
                time.sleep(args.delay)

            del gateway
        except KeyboardInterrupt:
            print("Process interrupted by user (Ctrl+C). Exiting gracefully.")
            break
        except:
            traceback.print_exc()
            time.sleep(args.delay)


if __name__ == "__main__":
    main()
