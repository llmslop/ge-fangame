add_subdirectory(extern)
add_library(ge-hal STATIC)

if(GE_HAL_STM32)
    target_sources(
        ge-hal
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include/ge-hal/app.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/include/ge-hal/stm/gpio.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/include/ge-hal/stm/rcc.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/include/ge-hal/stm/time.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/include/ge-hal/stm/uart.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/include/ge-hal/stm/sdram.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/include/ge-hal/stm/spi.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/include/ge-hal/stm/framebuffer.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/include/ge-hal/stm/dma2d.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/include/ge-hal/stm/rng.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/stm/app.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/stm/gpio.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/stm/time.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/stm/uart.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/stm/sdram.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/stm/spi.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/stm/framebuffer.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/stm/dma2d.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/stm/rng.cpp
    )
    target_link_libraries(ge-hal PUBLIC cmsis cmsis_device_f4)
    target_compile_definitions(ge-hal PUBLIC GE_HAL_STM32)
else()
    target_sources(
        ge-hal
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/pc/app.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/pc/gpu.cpp
    )
    find_package(SDL3 REQUIRED)
    target_link_libraries(ge-hal PUBLIC SDL3::SDL3)
    target_compile_definitions(ge-hal PUBLIC GE_HAL_PC)
endif()

target_include_directories(ge-hal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

function(ge_hal_add_link_sources target_name)
    if(GE_HAL_STM32)
        message(${CMAKE_CURRENT_FUNCTION_LIST_DIR})
        target_sources(
            ${target_name}
            PRIVATE
                ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/linker.ld
                ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/stm/startup.cpp
                ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/stm/syscalls.cpp
                ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/extern/cmsis_device_f4/Source/Templates/gcc/startup_stm32f429xx.s
        )
        target_link_options(
            ${target_name}
            PRIVATE
                $<$<C_COMPILER_ID:GNU>:-T${CMAKE_CURRENT_FUNCTION_LIST_DIR}/linker.ld>
        )
    endif()
endfunction()
