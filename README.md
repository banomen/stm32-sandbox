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

Если openocd пишет `Error: open failed` — ST-LINK не найден: проверьте кабель
(разъём mini-USB CN1) и драйвер ST-LINK (STSW-LINK009).

### CLion

Toolchain должен быть Windows-овый (System / MinGW), а не WSL: WSL-cmake
вызывает Windows-компилятор с путями `/mnt/c/...`, и сборка ломается.
Settings → Build → Toolchains → Default = System, генератор Ninja.
