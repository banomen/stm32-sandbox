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

Прошивка и отладка из CLion — готовые конфигурации «Плата 1» и «Плата 2»
(лежат в `.run/`, CLion подхватывает их сам). Один раз настроить:

1. Settings → Build, Execution, Deployment → CMake: включить профили `board1` и
   `board2` (они берутся из `CMakePresets.json`).
2. Settings → Build, Execution, Deployment → Embedded Development → OpenOCD Location:
   путь к настоящему `openocd.exe`, не к обёртке chocolatey, например
   `C:\ProgramData\chocolatey\lib\openocd\tools\install\bin\openocd.exe`.

Дальше выбрать конфигурацию «Плата 1» или «Плата 2»:

- Run (Shift+F10) — собрать и прошить, МК запускается;
- Debug (Shift+F9) — собрать, прошить и начать отладку с точками останова.

Конфигурация openocd для платы: `flash_n_debug/stm32f4discovery.cfg`.

## Ветка i2c: компьютер → UART → плата 1 → I2C → плата 2 → UART → компьютер

Нужно только: две STM32F4DISCOVERY, один USB-UART переходник и провода-дюпоны.

Плата 1 принимает строку (до `\n`, не длиннее 64 байт) и передаёт её по I2C
плате 2 (адрес `0x20`). Плата 2 отвечает на компьютер строкой `board2: <то, что пришло>`.
Зелёный светодиод мигает на каждое сообщение; красный на плате 1 — плата 2 не ответила.

Одна запись I2C: START, адрес, байт длины, байты сообщения, STOP. Плата 2 считает
сообщение готовым, когда пришло столько байт, сколько обещала длина. Каждая запись
начинает счёт заново, так что сбой портит только одно сообщение.

| Откуда                  | Куда                         | Назначение          |
|-------------------------|------------------------------|---------------------|
| TX переходника          | PA3 платы 1 (USART2 RX)      | компьютер → плата 1 |
| PB6 платы 1 (I2C1 SCL)  | PB6 платы 2                  | I2C SCL             |
| PB9 платы 1 (I2C1 SDA)  | PB9 платы 2                  | I2C SDA             |
| PA2 платы 2 (USART2 TX) | RX переходника               | плата 2 → компьютер |
| GND                     | GND обеих плат и переходника | общая земля         |

I2C: 100 кГц. Резисторы не нужны: PB6/PB9 на Discovery уже подтянуты к 3.3 В
на самой плате (это шина аудиокодека CS43L22, прошивка держит его в сбросе).
UART: 115200 8N1.

Переходник: если у него есть перемычка 3.3 В / 5 В — ставьте 3.3 В. Его VCC никуда
не подключайте, платы питаются от своих USB (ST-LINK).

Проверка с компьютера:

```
pip install pyserial
python tools/uart_i2c_test.py COM5
```
