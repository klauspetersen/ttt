set(CMAKE_VERBOSE_MAKEFILE ON)
cmake_minimum_required(VERSION 3.3)
project(ttt)

find_package(Threads)
find_package(PkgConfig)
pkg_check_modules(GLIB2 REQUIRED glib-2.0>=2.36.0)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0>=1.0)


include_directories(${GLIB2_INCLUDE_DIRS})
include_directories(/usr/include/libusb-1.0)
include_directories(src/extern/sigrok/include)
include_directories(src)
include_directories(src/extern/sigrok)
include_directories(src/extern/trace)
include_directories(src/extern/Catch/include)


set(CMAKE_C_COMPILER             "/usr/bin/clang-3.6")
set(CMAKE_C_FLAGS                "-Wall -v -std=c99 -fno-omit-frame-pointer")
set(CMAKE_C_FLAGS_DEBUG          "-g")
set(CMAKE_C_FLAGS_RELEASE        "-O4 -DNDEBUG")
set(CMAKE_C_FLAGS_TEST           "-g")

set(CMAKE_CXX_COMPILER             "/usr/bin/clang++-3.6")
set(CMAKE_CXX_FLAGS                "-Wall -v --std=c++14ss -msse4 -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG          "-g")
set(CMAKE_CXX_FLAGS_RELEASE        "-O4 -DNDEBUG")
set(CMAKE_CXX_FLAGS_TEST           "-g")


set(CMAKE_AR      "/usr/bin/llvm-ar")
set(CMAKE_LINKER  "/usr/bin/llvm-ld")
set(CMAKE_NM      "/usr/bin/llvm-nm")
set(CMAKE_OBJDUMP "/usr/bin/llvm-objdump")
set(CMAKE_RANLIB  "/usr/bin/llvm-ranlib")

file(GLOB TEST_SOURCES
        "src/tests/*.h"
        "src/tests/*.cpp"
        )

file(GLOB EXTERN_SOURCES
        "src/extern/sigrok/*.h"
        "src/extern/sigrok/*.c"
        )

file(GLOB EXTERN_HARDWARE_SOURCES
        "src/extern/sigrok/hardware/saleae-logic16/*.h"
        "src/extern/sigrok/hardware/saleae-logic16/*.c"
        )

set(SOURCE_FILES src/main.cpp src/sigrok_wrapper.c src/TransferObjectPool.cpp src/TransferObjectPool.h)

set(SOURCE_EXTERN_TRACE_SOURCES
        "src/extern/trace/raw_sampler.cpp"
        "src/extern/trace/raw_trace.cpp"
        "src/extern/trace/sw_decode.cpp"
        "src/extern/trace/sw_sampler.cpp"
        "src/extern/trace/sw_trace.cpp"
        "src/extern/trace/xble_common.cpp"
        "src/extern/trace/xble_decoder.cpp"
        "src/extern/trace/xble_link.cpp"
        "src/extern/trace/xble_rx_sampler.cpp"
        "src/extern/trace/xble_trace.cpp"
        "src/extern/trace/xble_tx_sampler.cpp"
        )


add_executable(ttt ${SOURCE_FILES} ${EXTERN_HARDWARE_SOURCES} ${EXTERN_SOURCES} ${SOURCE_EXTERN_TRACE_SOURCES})
set_target_properties(ttt PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build)
target_link_libraries (ttt ${GLIB2_LIBRARIES})
target_link_libraries (ttt ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries (ttt ${LIBUSB_LIBRARIES})
set_property(TARGET ttt PROPERTY CXX_STANDARD 11)
set_property(TARGET ttt PROPERTY C_STANDARD 99)



add_executable(tst ${TEST_SOURCES} src/TransferObjectPool.cpp src/TransferObjectPool.h src/tests/TransferObjectPoolTest.cpp)

target_compile_features(tst PRIVATE cxx_return_type_deduction)

set_target_properties(tst PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build)
target_link_libraries (tst ${GLIB2_LIBRARIES})
target_link_libraries (tst ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries (tst ${LIBUSB_LIBRARIES})
set_property(TARGET tst PROPERTY CXX_STANDARD 11)
set_property(TARGET tst PROPERTY C_STANDARD 99)
