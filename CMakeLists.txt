project(SysMon2)
cmake_minimum_required(VERSION 2.9)
include_directories(
    ./
    ./tree
    ./tree/kmap
    ./callbacks
    ./callbacks/comm
    ./general
    ./init
    ./kernel
    /usr/include
    /usr/local/include
    )

FILE(GLOB SYSMON_LIST
    "docs/*.h"
    "*.c" "*.h"
    "init/*.c" "init/*.h"
    "tree/*.c" "tree/*.h"
    "tree/kmap/*.c" "tree/kmap/*.h"
    "callbacks/*.c" "callbacks/*.h"
    "callbacks/comm/*.c" "callbacks/comm/*.h"
    "general/*.c" "general/*.h"
    "kernel/*.c" "kernel/*.h"
)
#list(REMOVE_ITEM SYSMON_LIST "${CMAKE_CURRENT_SOURCE_DIR}/main.c")
FILE(GLOB TEST_LIST
    "build/*.c"
)

option(ARM64_CRO "use arm64 crosstool" OFF)
option(MIPS64EL_CRO "use mips64el crosstool" OFF)
option(ALPHA64_CRO "use alpha64(sw) crosstool" OFF)
option(LOONGARCH64_CRO "use loongarch64 crosstool" OFF)
if(ARM64_CRO OR MIPS64EL_CRO OR ALPHA64_CRO OR LOONGARCH64_CRO)
    SET(CROSS_DIR "/opt/crossTools")
    if(ARM64_CRO)
        set(CMAKE_AR "${CROSS_DIR}/gcc-linaro-5.4.1-2017.01-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-ar")
        set(CMAKE_C_COMPILER "${CROSS_DIR}/gcc-linaro-5.4.1-2017.01-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-gcc")
        set(CMAKE_CXX_COMPILER "${CROSS_DIR}/gcc-linaro-5.4.1-2017.01-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-g++")
        set(CPU_TYPE_S "ARM64")
    #    set(CMAKE_CXX_LINK_EXECUTABLE "/opt/crossTools/gcc-linaro-5.4.1-2017.01-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-ld")
    elseif(MIPS64EL_CRO)
        set(CMAKE_AR "${CROSS_DIR}/cross-gcc-4.9.3-n64-loongson-rc6.1/bin/mips64el-loongson-linux-ar")
        set(CMAKE_C_COMPILER "${CROSS_DIR}/cross-gcc-4.9.3-n64-loongson-rc6.1/bin/mips64el-loongson-linux-gcc")
        set(CMAKE_CXX_COMPILER "${CROSS_DIR}/cross-gcc-4.9.3-n64-loongson-rc6.1/bin/mips64el-loongson-linux-g++")
        set(CPU_TYPE_S "MIPS64EL")
    #    set(CMAKE_CXX_LINK_EXECUTABLE "/opt/crossTools/cross-gcc-4.9.3-n64-loongson-rc6.1/bin/mips64el-loongson-linux-ld")
    elseif(ALPHA64_CRO)
        set(CMAKE_AR "${CROSS_DIR}/sw/swgcc830-mieee-opt-cross/usr/bin/sw_64sw6a-sunway-linux-gnu-ar")
        set(CMAKE_C_COMPILER "${CROSS_DIR}/sw/swgcc830-mieee-opt-cross/usr/bin/sw_64sw6a-sunway-linux-gnu-gcc")
        set(CMAKE_CXX_COMPILER "${CROSS_DIR}/sw/swgcc830-mieee-opt-cross/usr/bin/sw_64sw6a-sunway-linux-gnu-g++")
        set(CPU_TYPE_S "ALPHA64")
    #    set(CMAKE_CXX_LINK_EXECUTABLE "/opt/crossTools/sw/swgcc830-mieee-opt-cross/usr/bin/sw_64sw6a-sunway-linux-gnu-ld")
    elseif(LOONGARCH64_CRO)
        set(CMAKE_AR "${CROSS_DIR}/toolchain-loongarch64-linux-gnu-cross-830-rc1.0-2022-04-22/bin/loongarch64-linux-gnu-ar")
        set(CMAKE_C_COMPILER "${CROSS_DIR}/toolchain-loongarch64-linux-gnu-cross-830-rc1.0-2022-04-22/bin/loongarch64-linux-gnu-gcc")
        set(CMAKE_CXX_COMPILER "${CROSS_DIR}/toolchain-loongarch64-linux-gnu-cross-830-rc1.0-2022-04-22/bin/loongarch64-linux-gnu-g++")
        set(CPU_TYPE_S "LOONGARCH64")
    #    set(CMAKE_CXX_LINK_EXECUTABLE "/opt/crossTools/toolchain-loongarch64-linux-gnu-cross-830-rc1.0-2022-04-22/bin/loongarch64-linux-gnu-ld")
    endif()
endif()

#set(CMAKE_BUILD_TYPE "Release")
#set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
add_definitions(-D_GNU_SOURCE)      #解决pthread_tryjoin_np警告
link_libraries(pthread)
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
add_executable(test ${TEST_LIST})
#add_library(SysMon SHARED ${SYSMON_LIST})
add_executable(${PROJECT_NAME} ${SYSMON_LIST})
