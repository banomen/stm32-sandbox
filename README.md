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

```
cmake -G Ninja -B build
cmake --build build
```

Результат: `build/firmware.elf` (+ `.bin`, `.hex`).

### Прошивка (ST-LINK на плате STM32F4DISCOVERY)

```
cmake --build build --target flash
```

`--target reset` — только перезапустить МК.

Если openocd пишет `Error: open failed` — ST-LINK не найден: проверьте кабель
(разъём mini-USB CN1) и драйвер ST-LINK (STSW-LINK009).

### CLion

Toolchain должен быть Windows-овый (System / MinGW), а не WSL: WSL-cmake
вызывает Windows-компилятор с путями `/mnt/c/...`, и сборка ломается.
Settings → Build → Toolchains → Default = System, генератор Ninja.
