cmake_minimum_required(VERSION 3.6)

include(./modm/repo.cmake)

project(purpledrop)

set(PRE_CONFIGURE_FILE "${CMAKE_SOURCE_DIR}/src/version.cpp.in")
set(POST_CONFIGURE_FILE "${CMAKE_SOURCE_DIR}/src/version.cpp")
message(${PRE_CONFIGURE_FILE})
include(version.cmake)

set(CMAKE_BUILD_TYPE Debug)

set(PERIPH lib/STM32F4xx_StdPeriph_Driver)
set(USB lib/STM32_USB_OTG_Driver)
set(USBD lib/STM32_USB_Device_Library)

file(GLOB APP_SOURCE_FILES 
    src/*.c
    src/*.cpp
    src/usb/*.c
    src/usb/*.cpp
    src/app/*.cpp
    src/driver/*.c
    src/driver/*.cpp
    ${USB}/src/usb_core.c
	${USB}/src/usb_dcd_int.c
	${USB}/src/usb_dcd.c
	${USBD}/Class/cdc/src/usbd_cdc_core.c
	${USBD}/Core/src/usbd_core.c
	${USBD}/Core/src/usbd_ioreq.c
	${USBD}/Core/src/usbd_req.c)

list(APPEND SOURCE_FILES ${APP_SOURCE_FILES})

add_definitions(-DUSE_USB_OTG_FS -DSTM32F413_423xx)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    src
    src/driver
    src/usb
    src/app
    ${PERIPH}/inc
    ${USB}/inc
    ${USBD}/Core/inc
    ${USBD}/Class/cdc/inc
)

add_executable(${CMAKE_PROJECT_NAME} ${SOURCE_FILES})
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES SUFFIX ".elf")
add_dependencies(${CMAKE_PROJECT_NAME} update_version)

#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T ../../memory.ld")

add_custom_target(${CMAKE_PROJECT_NAME}.bin ALL DEPENDS ${CMAKE_PROJECT_NAME} COMMAND ${CMAKE_OBJCOPY} -Obinary ${CMAKE_PROJECT_NAME}.elf ${CMAKE_PROJECT_NAME}.bin)
add_custom_target(${CMAKE_PROJECT_NAME}.hex ALL DEPENDS ${CMAKE_PROJECT_NAME} COMMAND ${CMAKE_OBJCOPY} -Oihex ${CMAKE_PROJECT_NAME}.elf ${CMAKE_PROJECT_NAME}.hex)
add_custom_target(${CMAKE_PROJECT_NAME}.lss ALL DEPENDS ${CMAKE_PROJECT_NAME} COMMAND ${CMAKE_OBJDUMP}  -x -s -S -l -w ${CMAKE_PROJECT_NAME}.elf > ${CMAKE_PROJECT_NAME}.lss)

