message(CHECK_START "\tSources")


# Название прошивки определяется названием проекта (определено в основном сценарии cmake)
# Какую плату собирать: 1 или 2. У каждой платы своя папка сборки
# (пресеты board1 / board2 в CMakePresets.json).
set(BOARD 1 CACHE STRING "Плата: 1 или 2")
set_property(CACHE BOARD PROPERTY STRINGS 1 2)
if(NOT BOARD MATCHES "^[12]$")
    message(FATAL_ERROR "BOARD должен быть 1 или 2, сейчас: ${BOARD}")
endif()
message(STATUS "		Board: ${BOARD}")

# Код, общий для обеих плат
set(COMMON_SOURCES
    i2c_slave.cpp
)

add_executable(${CMAKE_PROJECT_NAME}
    board${BOARD}.cpp
    ${COMMON_SOURCES}
)

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Папки библиотек должны включаться в основной сценарии заранее
target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
    libopencm3
)

set_target_properties(${CMAKE_PROJECT_NAME}
        PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

# Готовая прошивка копируется в корень папки сборки (firmware.elf) — её
# использует цель flash и отладчик (cortex-debug / CLion). Рядом кладутся .bin и .hex.
add_custom_command(TARGET ${CMAKE_PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_BINARY_DIR}/firmware.elf
    COMMAND ${OBJCOPY_UTIL} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_BINARY_DIR}/firmware.bin
    COMMAND ${OBJCOPY_UTIL} -O ihex $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_BINARY_DIR}/firmware.hex
    COMMAND ${SIZE_UTIL} $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
    VERBATIM
)


message(CHECK_PASS "Done")
