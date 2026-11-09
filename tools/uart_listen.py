"""Печатает всё, что плата 2 присылает на компьютер по UART.

Сообщения в плату 1 отправляются отдельно, через USB-I2C переходник
(запись на адрес 0x10), см. README.

    pip install pyserial
    python tools/uart_listen.py COM5
"""

import argparse

import serial


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("port", help="COM-порт USB-UART переходника, например COM5 или /dev/ttyUSB0")
    parser.add_argument("--baud", type=int, default=115200)
    args = parser.parse_args()

    with serial.Serial(args.port, args.baud, timeout=0.5) as port:
        print("Жду сообщения, Ctrl+C — выход")
        try:
            while True:
                line = port.readline()
                if line:
                    print(line.decode(errors="replace").rstrip())
        except KeyboardInterrupt:
            pass


if __name__ == "__main__":
    main()
