import argparse
import sys
import time

try:
    import msvcrt
except ImportError:
    msvcrt = None

try:
    import serial
except ImportError:
    serial = None


ARROW_MAP = {
    b"H": b"U",
    b"P": b"D",
    b"K": b"L",
    b"M": b"R",
}


def parse_args():
    parser = argparse.ArgumentParser(description="Send PC arrow keys to STM32 snake over serial.")
    parser.add_argument("port", help="Serial port, for example COM3")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate, default 115200")
    return parser.parse_args()


def main():
    if msvcrt is None:
        print("This helper currently supports Windows consoles only.")
        return 1
    if serial is None:
        print("pyserial is required. Install it with: pip install pyserial")
        return 1

    args = parse_args()
    with serial.Serial(args.port, args.baud, timeout=0) as ser:
        try:
            ser.dtr = False
            ser.rts = False
        except IOError:
            pass
        print(f"Connected to {args.port} at {args.baud}.")
        print("Use arrow keys to move, Enter to start/restart, Esc to quit.")
        while True:
            if not msvcrt.kbhit():
                time.sleep(0.005)
                continue

            ch = msvcrt.getch()
            if ch in (b"\x00", b"\xe0"):
                code = msvcrt.getch()
                cmd = ARROW_MAP.get(code)
                if cmd:
                    ser.write(cmd)
                    ser.flush()
            elif ch == b"\r":
                ser.write(b"S")
                ser.flush()
            elif ch == b"\x1b":
                print("Exit.")
                break
    return 0


if __name__ == "__main__":
    sys.exit(main())
