cmake_minimum_required(VERSION 3.3)
project(ttt)

set(CMAKE_VERBOSE_MAKEFILE ON)
find_package(Threads)
find_package(PkgConfig)
pkg_check_modules(GLIB2 REQUIRED glib-2.0>=2.36.0)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0>=1.0)


include_directories(${GLIB2_INCLUDE_DIRS})
include_directories(/usr/include/libusb-1.0)
include_directories(src/extern/include)
include_directories(src/extern)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

file(GLOB EXTERN_SOURCES
        "src/extern/*.h"
        "src/extern/*.c"
        )

file(GLOB EXTERN_HARDWARE_SOURCES
        "src/extern/hardware/saleae-logic16/*.h"
        "src/extern/hardware/saleae-logic16/*.c"
        )

set(SOURCE_FILES src/main.cpp)

add_executable(ttt ${SOURCE_FILES} ${EXTERN_HARDWARE_SOURCES} ${EXTERN_SOURCES})
set_target_properties(ttt PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build)
target_link_libraries (ttt ${GLIB2_LIBRARIES})
target_link_libraries (ttt ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries (ttt ${LIBUSB_LIBRARIES})
set_property(TARGET ttt PROPERTY CXX_STANDARD 11)
set_property(TARGET ttt PROPERTY C_STANDARD 99)