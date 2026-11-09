message(CHECK_START "\tarm-none-eabi- toolchain ")


list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR})

set(ENV{CMAKE_EXPORT_COMPILE_COMMANDS} 1)


set(CMAKE_SYSTEM_NAME  Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)
set(CMAKE_CROSSCOMPILING 1)

set(TOOLCHAIN_PREFIX arm-none-eabi-)



if(CMAKE_HOST_WIN32)
    set(CMAKE_EXECUTABLE_SUFFIX .exe)
endif()

find_program(TOOLCHAIN_GCC arm-none-eabi-gcc)
if(NOT TOOLCHAIN_GCC)
    message(FATAL_ERROR "arm-none-eabi-gcc not found in PATH. CLion: используйте Windows-тулчейн, а не WSL (см. README).")
endif()
get_filename_component(TOOLCHAIN_DIR ${TOOLCHAIN_GCC} DIRECTORY)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER ${TOOLCHAIN_DIR}/arm-none-eabi-gcc${CMAKE_EXECUTABLE_SUFFIX})
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_DIR}/arm-none-eabi-gcc${CMAKE_EXECUTABLE_SUFFIX})
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_DIR}/arm-none-eabi-g++${CMAKE_EXECUTABLE_SUFFIX})

set(SIZE_UTIL ${TOOLCHAIN_DIR}/arm-none-eabi-size${CMAKE_EXECUTABLE_SUFFIX})
set(OBJDUMP_UTIL ${TOOLCHAIN_DIR}/arm-none-eabi-objdump${CMAKE_EXECUTABLE_SUFFIX})
set(OBJCOPY_UTIL ${TOOLCHAIN_DIR}/arm-none-eabi-objcopy${CMAKE_EXECUTABLE_SUFFIX})


set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)


# Настройки компилятора
add_compile_options(
     -ggdb3
     -Os

     -fdata-sections
     -ffunction-sections

     -fno-common

     -fno-exceptions

     -Wall
     -Wextra
     -Wshadow
     -Wdouble-promotion
     -Wundef
     -Wconversion
)

# Настройки компоновщика
add_link_options(
     -nostartfiles

     -specs=nano.specs
     -specs=nosys.specs
     -lc
     -lm
     -lnosys

     -Wl,--gc-sections
)


message(CHECK_PASS "Done")
