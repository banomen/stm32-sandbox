"""Проверка цепочки компьютер -UART-> плата 1 -SPI-> плата 2 -UART-> компьютер.

Хватает одного USB-UART переходника: его TX на PA3 платы 1, его RX на PA2 платы 2.

    pip install pyserial
    python tools/uart_spi_test.py COM5
"""

import argparse

import serial


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("port", help="COM-порт переходника, например COM5 или /dev/ttyUSB0")
    parser.add_argument("--baud", type=int, default=115200)
    args = parser.parse_args()

    with serial.Serial(args.port, args.baud, timeout=0.5) as port:
        print("Вводите строки, Ctrl+C — выход")
        while True:
            try:
                line = input("> ")
            except (KeyboardInterrupt, EOFError):
                break
            if not line:
                continue

            port.reset_input_buffer()
            port.write((line + "\n").encode())

            # Строка длиннее 64 байт уходит несколькими сообщениями, поэтому читаем все ответы
            answers = []
            while answer := port.readline():
                answers.append(answer.decode(errors="replace").rstrip())
            print("\n".join(answers) if answers else "(нет ответа)")


if __name__ == "__main__":
    main()
