# Шаблон проекта программы для МК

## Требуемые инструментальные средства:

- arm-none-eabi-gcc
- gdb-multiarch (arm-none-eabi-gdb)
- make (make + ninja-build)
- cmake
- openocd
- doxygen (для документирования программы)


## Рекомендуемый редактор (IDE)

Visual Studio Code c плагинами ():

- C/C++;
- CMake Tools;
- Cortex-Debug.

## Настройка и сборка проекта

Нужны в PATH: `arm-none-eabi-gcc`, `cmake`, `ninja`, `make`, `python`, `openocd`.
Исходники libopencm3 скачиваются при первой настройке в `.cmake_cache/libopencm3`
и собираются один раз (около минуты).

В проекте две прошивки — для платы 1 и платы 2. Плата выбирается переменной
`BOARD`, у каждой своя папка сборки:

```
cmake --preset board1
cmake --build build/board1
cmake --preset board2
cmake --build build/board2
```

Результат: `build/board1/firmware.elf` и `build/board2/firmware.elf` (+ `.bin`, `.hex`).
Код плат: `src/board1.cpp`, `src/board2.cpp`, общее — `src/common.hpp`.

### Прошивка (ST-LINK на плате STM32F4DISCOVERY)

```
cmake --build build/board1 --target flash
```

`--target reset` — только перезапустить МК.

Платы прошиваются по очереди: подключите по USB (CN1) только ту плату, которую
шьёте, иначе openocd возьмёт первый попавшийся ST-LINK.

Если openocd пишет `Error: open failed` — ST-LINK не найден: проверьте кабель
(разъём mini-USB CN1) и драйвер ST-LINK (STSW-LINK009).

### CLion

Toolchain должен быть Windows-овый (System / MinGW), а не WSL: WSL-cmake
вызывает Windows-компилятор с путями `/mnt/c/...`, и сборка ломается.
Settings → Build → Toolchains → Default = System, генератор Ninja.

## Ветка i2c: компьютер → I2C → плата 1 → I2C → плата 2 → UART → компьютер

Компьютер пишет сообщение (до 64 байт, одна транзакция записи) плате 1 на адрес
`0x10` через USB-I2C переходник (CH341, FT232H, MCP2221 и т.п., логика 3.3 В).
Плата 1 пересылает его по второй шине I2C плате 2 (адрес `0x20`).
Плата 2 отвечает на компьютер по UART строкой `board2: <то, что пришло>`.
Зелёный светодиод мигает на каждое сообщение; красный на плате 1 — плата 2 не ответила.

Подключение (обе платы — STM32F4DISCOVERY):

| Откуда                  | Куда                         | Назначение                      |
|-------------------------|------------------------------|---------------------------------|
| SCL USB-I2C переходника | PB10 платы 1 (I2C2 SCL)      | компьютер → плата 1             |
| SDA USB-I2C переходника | PB11 платы 1 (I2C2 SDA)      | компьютер → плата 1             |
| PA8 платы 1 (I2C3 SCL)  | PB10 платы 2 (I2C2 SCL)      | плата 1 → плата 2               |
| PC9 платы 1 (I2C3 SDA)  | PB11 платы 2 (I2C2 SDA)      | плата 1 → плата 2               |
| PA2 платы 2 (USART2 TX) | RX USB-UART переходника      | плата 2 → компьютер             |
| GND                     | GND плат и переходников      | общая земля                     |

I2C: 100 кГц. На каждую шину нужны подтяжки SCL и SDA к 3.3 В (4.7 кОм);
внутренние подтяжки МК включены, но они слабые и годятся только для коротких проводов.
UART: 115200 8N1.

Проверка с компьютера: в одном окне читаем UART

```
pip install pyserial
python tools/uart_listen.py COM5
```

и отправляем байты в плату 1 программой своего I2C-переходника. Например, на Linux
с переходником, который виден как `/dev/i2c-1`:

```
i2ctransfer -y 1 w5@0x10 0x68 0x65 0x6c 0x6c 0x6f
```

В окне UART должно появиться `board2: hello`.
