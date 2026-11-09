message(CHECK_START "\tLibopencm3")


if(NOT DEVICE)
  message(FATAL_ERROR "\t\tDevice must be defined at project level!\tExample: set(DEVICE stm32f407vg).")
endif()


#===============================================================================
# Python нужен для скриптов libopencm3 (genlink.py, irq2nvic_h).
# На Windows интерпретатор обычно называется python, а не python3,
# поэтому скрипты всегда запускаются явно через найденный интерпретатор.
find_package(Python3 REQUIRED COMPONENTS Interpreter)

find_program(MAKE_PROGRAM NAMES make mingw32-make gmake REQUIRED)

# Makefile libopencm3 вызывает sh, rm, mkdir. На Windows берём их из Git.
if(CMAKE_HOST_WIN32)
  find_program(UNIX_SH sh.exe REQUIRED PATHS
    "$ENV{ProgramFiles}/Git/usr/bin"
    "$ENV{LOCALAPPDATA}/Programs/Git/usr/bin"
  )
  get_filename_component(UNIX_TOOLS_DIR ${UNIX_SH} DIRECTORY)
  file(TO_NATIVE_PATH "${UNIX_TOOLS_DIR}" UNIX_TOOLS_DIR)
  set(ENV{PATH} "${UNIX_TOOLS_DIR};$ENV{PATH}")
endif()


#===============================================================================
# Исходники libopencm3 скачиваются один раз в .cmake_cache/libopencm3
# и используются всеми папками сборки (build/*, cmake-build-*).
include(FetchContent)

set(LOCM3_CACHE_DIR "${CMAKE_SOURCE_DIR}/.cmake_cache/libopencm3")
if(EXISTS "${LOCM3_CACHE_DIR}/Makefile")
  # Уже скачано: без этого FetchContent в новой папке сборки клонирует заново
  # и стирает собранную библиотеку.
  set(FETCHCONTENT_SOURCE_DIR_LLIBOPENCM3 "${LOCM3_CACHE_DIR}")
endif()

FetchContent_Declare(llibopencm3
  GIT_REPOSITORY https://github.com/libopencm3/libopencm3
  GIT_TAG master
  GIT_SHALLOW TRUE
  SOURCE_DIR "${LOCM3_CACHE_DIR}"
  UPDATE_DISCONNECTED TRUE # не ходить в сеть при каждой конфигурации
)

FetchContent_MakeAvailable(llibopencm3)
FetchContent_GetProperties(llibopencm3)


include(libopencm3_query.cmake) # Query defines and parameters based on DEVICE variable


#===============================================================================
# Сборка статической библиотеки libopencm3 для нужного семейства
set(LOCM3_STATIC_LIB ${LOCM3_STATIC_LIB_PREFIX}${LOCM3_DEVICE_SERIE}.a)

if(NOT EXISTS ${LOCM3_STATIC_LIB})
  # Заголовки прерываний генерируем сами: в Makefile скрипт вызывается
  # через "#!/usr/bin/env python3", что не работает на Windows.
  file(GLOB_RECURSE LOCM3_IRQ_JSONS
    RELATIVE ${llibopencm3_SOURCE_DIR}
    ${llibopencm3_SOURCE_DIR}/include/libopencm3/${LOCM3_TARGET}/irq.json
  )
  foreach(irq_json IN LISTS LOCM3_IRQ_JSONS)
    execute_process(
      COMMAND ${Python3_EXECUTABLE} scripts/irq2nvic_h ./${irq_json}
      WORKING_DIRECTORY ${llibopencm3_SOURCE_DIR}
      RESULT_VARIABLE res
    )
    if(NOT res EQUAL 0)
      message(FATAL_ERROR "\t\tirq2nvic_h failed for ${irq_json}")
    endif()
  endforeach()

  message(NOTICE "\t\tRun libopencm3 make (${LOCM3_TARGET}), это займёт около минуты...")
  execute_process(
    COMMAND ${MAKE_PROGRAM} lib TARGETS=${LOCM3_TARGET}
    WORKING_DIRECTORY ${llibopencm3_SOURCE_DIR}
    RESULT_VARIABLE res
  )
  if(NOT res EQUAL 0 OR NOT EXISTS ${LOCM3_STATIC_LIB})
    message(FATAL_ERROR "\t\tlibopencm3 build failed (${LOCM3_TARGET})")
  endif()
endif()


#===============================================================================
add_library(libopencm3 INTERFACE)

target_compile_definitions(libopencm3 INTERFACE ${LOCM3_DEVICE_DEFINES})
target_compile_options(libopencm3 INTERFACE ${MCPU} ${MTHUMB} ${MFPU} ${MFLOAT_ABI})
target_include_directories(libopencm3 INTERFACE ${LOCM3_INCLUDE_DIR})
target_link_libraries(libopencm3 INTERFACE ${LOCM3_STATIC_LIB})
target_link_options(libopencm3 INTERFACE -T${LOCM3_LINKER_GENERATED} ${MCPU} ${MTHUMB} ${MFPU} ${MFLOAT_ABI})


message(CHECK_PASS "Done")
