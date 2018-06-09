cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()


project(vitabright)
include("${VITASDK}/share/vita.cmake" REQUIRED)

option(ENABLE_LOGGING "ENABLE_LOGGING" OFF)
if (ENABLE_LOGGING)
  add_definitions(-DENABLE_LOGGING=1)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -fno-pic -fno-PIC -std=gnu99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")

include_directories(
)

link_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(vitabright
  main.c
  parser.c
  log.c
)


target_link_libraries(vitabright
  SceDisplayForDriver_stub
  taihenForKernel_stub
  SceIofilemgrForDriver_stub
  SceOledForDriver_stub
  #k
  gcc
  SceSysclibForDriver_stub
  SceSysmemForDriver_stub
  #SceLibKernel_stub
)

set_target_properties(vitabright
  PROPERTIES LINK_FLAGS "-nostdlib"
  COMPILE_FLAGS "-D__VITA_KERNEL__"
)

vita_create_self(vitabright.skprx vitabright
  UNSAFE
  CONFIG ${CMAKE_SOURCE_DIR}/vitabright.yml
)

