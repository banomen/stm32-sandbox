set(LOCM3_DEVICE_SCRIPT ${llibopencm3_SOURCE_DIR}/scripts/genlink.py)
set(LOCM3_DEVICE_DATABASE ${llibopencm3_SOURCE_DIR}/ld/devices.data)
set(LOCM3_LINKER_TEMPLATE ${llibopencm3_SOURCE_DIR}/ld/linker.ld.S)
set(LOCM3_LINKER_GENERATED ${CMAKE_BINARY_DIR}/generated.${DEVICE}.ld)
set(LOCM3_INCLUDE_DIR ${llibopencm3_SOURCE_DIR}/include)
set(LOCM3_STATIC_LIB_PREFIX ${llibopencm3_SOURCE_DIR}/lib/libopencm3_)

# Values debug print
message(VERBOSE "\n\t\tVerbose:")
message(VERBOSE "\t\tLibopencm3 device script: ${LOCM3_DEVICE_SCRIPT}")
message(VERBOSE "\t\tLibopencm3 device database: ${LOCM3_DEVICE_DATABASE}")
message(VERBOSE "\t\tLibopencm3 linker template: ${LOCM3_LINKER_TEMPLATE}")
message(VERBOSE "\t\tLibopencm3 linker script: ${LOCM3_LINKER_GENERATED}")
message(VERBOSE "\t\tLibopencm3 include dir: ${LOCM3_INCLUDE_DIR}")
message(VERBOSE "\t\tLibopencm3 static lib prefix: ${LOCM3_STATIC_LIB_PREFIX}")


message(NOTICE "\t\tQuery device info and generate linker script")

function(locm3_genlink what out_var)
  execute_process(
    COMMAND ${Python3_EXECUTABLE} ${LOCM3_DEVICE_SCRIPT} ${LOCM3_DEVICE_DATABASE} ${DEVICE} ${what}
    OUTPUT_VARIABLE out
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE res
  )
  if(NOT res EQUAL 0 OR out STREQUAL "")
    message(FATAL_ERROR "\t\tgenlink.py ${what} failed for ${DEVICE}")
  endif()
  set(${out_var} "${out}" PARENT_SCOPE)
endfunction()

locm3_genlink(CPPFLAGS LOCM3_DEVICE_DEFINES_STR)  # "-DSTM32F4 -DSTM32F4CCM -DSTM32F407VG"
locm3_genlink(CPU MCPU)
locm3_genlink(FPU FPU_STR)
locm3_genlink(DEFS DEVDEFS)

# Список макросов: каждый -D отдельным элементом
separate_arguments(LOCM3_DEVICE_DEFINES UNIX_COMMAND "${LOCM3_DEVICE_DEFINES_STR}")

# Генерация скрипта компоновщика
separate_arguments(GEN_LD UNIX_COMMAND "${DEVDEFS}")
execute_process(
  COMMAND ${CMAKE_C_COMPILER} ${GEN_LD} -P -E ${LOCM3_LINKER_TEMPLATE} -o ${LOCM3_LINKER_GENERATED}
  RESULT_VARIABLE res
)
if(NOT res EQUAL 0)
  message(FATAL_ERROR "\t\tLinker script generation failed")
endif()

# Values debug print
message(VERBOSE "\n\t\tVerbose:")
message(VERBOSE "\t\tDevice defines: ${LOCM3_DEVICE_DEFINES}")
message(VERBOSE "\t\tLd script generation parameters: ${GEN_LD}")


message(NOTICE "\t\tMake target definitions")
list(GET LOCM3_DEVICE_DEFINES 0 LOCM3_DEVICE_SERIE)        # -DSTM32F4
string(REGEX REPLACE "^-D" "" LOCM3_DEVICE_SERIE ${LOCM3_DEVICE_SERIE})
string(TOLOWER ${LOCM3_DEVICE_SERIE} LOCM3_DEVICE_SERIE)  # stm32f4
string(REPLACE stm32 stm32/ LOCM3_TARGET ${LOCM3_DEVICE_SERIE}) # stm32/f4
# Values debug print
message(VERBOSE "\n\t\tVerbose:")
message(VERBOSE "\t\tLibopencm3 build target: ${LOCM3_TARGET}")
message(VERBOSE "\t\tLibopencm3 linker script: ${LOCM3_LINKER_GENERATED}")


set(THUMB_CORES cortex-m0 cortex-m0plus cortex-m3 cortex-m4 cortex-m7)
if(MCPU IN_LIST THUMB_CORES)
  set(MTHUMB "-mthumb")
endif()
string(PREPEND MCPU "-mcpu=")


if(FPU_STR STREQUAL "soft")
  set(MFLOAT_ABI "-msoft-float")
elseif(FPU_STR STREQUAL "hard-fpv4-sp-d16")
  set(MFLOAT_ABI "-mfloat-abi=hard")
  set(MFPU "-mfpu=fpv4-sp-d16")
elseif(FPU_STR STREQUAL "hard-fpv5-sp-d16")
  set(MFLOAT_ABI "-mfloat-abi=hard")
  set(MFPU "-mfpu=fpv5-sp-d16")
else()
  message(WARNING "\t\tFloat ABI not defined!")
endif()
