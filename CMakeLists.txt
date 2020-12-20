cmake_minimum_required(VERSION 2.8)

if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if (DEFINED ENV{VITASDK})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
    else ()
        message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
    endif ()
endif ()


project(vitabright)
include("${VITASDK}/share/vita.cmake" REQUIRED)

option(ENABLE_LOGGING "ENABLE_LOGGING" OFF)
if (ENABLE_LOGGING)
    add_definitions(-DENABLE_LOGGING=1)
endif ()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -fno-pic -fno-PIC -std=gnu99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")

include_directories(
)

link_directories(
        ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(vitabright
        main.c
        log.c
        oled/parser.c
        oled/hooks.c
        lcd/hooks.c
        )


target_link_libraries(vitabright
        SceDisplayForDriver_stub
        taihenForKernel_stub
        taihenModuleUtils_stub
        SceIofilemgrForDriver_stub
        #k
        gcc
        SceSysclibForDriver_stub
        SceSysmemForDriver_stub
        SceSysrootForDriver_stub
        #SceLibKernel_stub
        )

set_target_properties(vitabright
        PROPERTIES LINK_FLAGS "-nostdlib"
        COMPILE_FLAGS "-D__VITA_KERNEL__"
        )

vita_create_self(vitabright.skprx vitabright
        CONFIG ${CMAKE_SOURCE_DIR}/module.yml
        UNSAFE
        )

vita_create_stubs(vitabright-stubs vitabright module.yml
        KERNEL
        )

install(DIRECTORY ${CMAKE_BINARY_DIR}/vitabright-stubs/
        DESTINATION lib
        FILES_MATCHING PATTERN "*.a"
        )

set(PSVITAIP 192.168.1.10)

add_custom_target(send
        COMMAND curl --ftp-method nocwd -T vitabright.skprx ftp://${PSVITAIP}:1337/ur0:/tai/vitabright.skprx
        DEPENDS vitabright.skprx
        )