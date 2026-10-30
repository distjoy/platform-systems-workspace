cmake_minimum_required(VERSION 3.22)

project(baremetal_telemtry_pipeline C CXX ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS "We are on a ${CMAKE_SYSTEM_NAME} system")
message(STATUS "The host processor is ${CMAKE_HOST_SYSTEM_PROCESSOR}")

set(MCU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

# 1. Declare the executable target
add_executable(${PROJECT_NAME} 
    main.cpp
    I2CDriver.cpp
    I2CDriver.hpp
    SpiMaster.cpp
    SpiMaster.hpp
    ../../Drivers/CMSIS/cmsis_f4/Source/Templates/gcc/startup_stm32f411xe.s
    ../../Drivers/CMSIS/cmsis_f4/Source/Templates/system_stm32f4xx.c)

# 2. Compile definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE 
    STM32F411xE
)

# 3. Compile options (applied directly to target)
target_compile_options(${PROJECT_NAME} PRIVATE
    ${MCU_FLAGS}
    -Wall
    -Wextra
    -fdata-sections
    -ffunction-sections
    -fno-exceptions
    -fno-rtti
)

# 4. Link options (applied directly to target)
target_link_options(${PROJECT_NAME} PRIVATE
    ${MCU_FLAGS}
    -Wl,--gc-sections
    --specs=nano.specs
    --specs=nosys.specs
    -T${CMAKE_CURRENT_SOURCE_DIR}/STM32F411RETX_FLASH.ld
)

# 5. Include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    ../../Drivers/CMSIS/cmsis_core/CMSIS/Core/Include
    ../../Drivers/CMSIS/cmsis_f4/Include
)