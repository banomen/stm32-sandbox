message(CHECK_START "\tFlash and debug tools")


set(PROGRAMMER stlink)

# Openocd target config selection (incomplete)
if(DEVICE MATCHES "stm32f0")
    set(TARGET_DEVICE stm32f0x)
elseif(DEVICE MATCHES "stm32f1")
    set(TARGET_DEVICE stm32f1x)
elseif(DEVICE MATCHES "stm32f2")
    set(TARGET_DEVICE stm32f2x)
elseif(DEVICE MATCHES "stm32f3")
    set(TARGET_DEVICE stm32f3x)
elseif(DEVICE MATCHES "stm32f4")
    set(TARGET_DEVICE stm32f4x)
elseif(DEVICE MATCHES "stm32f7")
    set(TARGET_DEVICE stm32f7x)
elseif(DEVICE MATCHES "stm32g0")
    set(TARGET_DEVICE stm32g0x)
elseif(DEVICE MATCHES "stm32g4")
    set(TARGET_DEVICE stm32g4x)
elseif(DEVICE MATCHES "stm32h7")
    set(TARGET_DEVICE stm32h7x)
elseif(DEVICE MATCHES "stm32l4")
    set(TARGET_DEVICE stm32l4x)
elseif(DEVICE MATCHES "stm32l5")
    set(TARGET_DEVICE stm32l5x)
else()
    message(WARNING "\tOpenocd target: unknown target!")
endif()

# Openocd interface and target strings forming
string(APPEND TARGET_DEVICE ".cfg")
string(APPEND PROGRAMMER ".cfg")
string(PREPEND TARGET_DEVICE "target/")
string(PREPEND PROGRAMMER "interface/")

message(VERBOSE "\tOpenocd target: " ${TARGET_DEVICE})


#===============================================================================
# Прошивка через ST-LINK (на STM32F4DISCOVERY он встроенный):
#   cmake --build build --target flash
set(FIRMWARE_FILE ${CMAKE_BINARY_DIR}/firmware.elf)

find_program(OPENOCD_EXECUTABLE openocd)
if(NOT OPENOCD_EXECUTABLE)
    message(WARNING "	openocd not found, target 'flash' will not work")
    set(OPENOCD_EXECUTABLE openocd)
endif()

set(OPENOCD_ARGS -f ${PROGRAMMER} -f ${TARGET_DEVICE} -c "program ${FIRMWARE_FILE} verify reset exit")

add_custom_target(flash
    COMMAND ${OPENOCD_EXECUTABLE} ${OPENOCD_ARGS}
    DEPENDS ${PROJECT_NAME}
    COMMENT "Flashing ${FIRMWARE_FILE}"
    USES_TERMINAL
    VERBATIM
)

# Сброс МК без перепрошивки
add_custom_target(reset
    COMMAND ${OPENOCD_EXECUTABLE} -f ${PROGRAMMER} -f ${TARGET_DEVICE} -c "init" -c "reset run" -c "shutdown"
    USES_TERMINAL
    VERBATIM
)


#===============================================================================
# Альтернатива: cmake --install build  (тоже прошивает)
install(CODE "
    message(STATUS \"Flashing firmware\")
    execute_process(
        COMMAND \"${OPENOCD_EXECUTABLE}\" -f ${PROGRAMMER} -f ${TARGET_DEVICE}
                -c \"program ${FIRMWARE_FILE} verify reset exit\"
        RESULT_VARIABLE res
    )
    if(NOT res EQUAL 0)
        message(FATAL_ERROR \"Flashing failed!\")
    endif()
    message(STATUS \"Flashing complete\")
")


message(CHECK_PASS "Done")
