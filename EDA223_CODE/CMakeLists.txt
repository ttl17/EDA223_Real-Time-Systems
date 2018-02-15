cmake_minimum_required(VERSION 3.9)
project(EDA223_CODE)

set(CMAKE_CXX_STANDARD 11)

include_directories(TinyTimber/device)
include_directories(TinyTimber/device/inc)
include_directories(TinyTimber/driver)
include_directories(TinyTimber/driver/inc)
include_directories(TinyTimber/driver/src)

add_executable(EDA223_CODE
        TinyTimber/device/inc/core_cm4.h
        TinyTimber/device/inc/core_cm4_simd.h
        TinyTimber/device/inc/core_cmFunc.h
        TinyTimber/device/inc/core_cmInstr.h
        TinyTimber/device/inc/stm32f4xx.h
        TinyTimber/device/inc/system_stm32f4xx.h
        TinyTimber/driver/inc/stm32f4xx_can.h
        TinyTimber/driver/inc/stm32f4xx_dac.h
        TinyTimber/driver/inc/stm32f4xx_gpio.h
        TinyTimber/driver/inc/stm32f4xx_rcc.h
        TinyTimber/driver/inc/stm32f4xx_tim.h
        TinyTimber/driver/inc/stm32f4xx_usart.h
        TinyTimber/driver/src/stm32f4xx_can.c
        TinyTimber/driver/src/stm32f4xx_dac.c
        TinyTimber/driver/src/stm32f4xx_gpio.c
        TinyTimber/driver/src/stm32f4xx_rcc.c
        TinyTimber/driver/src/stm32f4xx_tim.c
        TinyTimber/driver/src/stm32f4xx_usart.c
        TinyTimber/application.c
        TinyTimber/canTinyTimber.c
        TinyTimber/canTinyTimber.h
        TinyTimber/sciTinyTimber.c
        TinyTimber/sciTinyTimber.h
        TinyTimber/startup.c
        TinyTimber/TinyTimber.c
        TinyTimber/TinyTimber.h)
