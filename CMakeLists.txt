project(SysMon)
cmake_minimum_required(VERSION 2.9)
include_directories(
    ./
    ./cbtree
    ./cbtree/kmap
    ./callbacks
    ./init
    /usr/include
    /usr/local/include
    )

FILE(GLOB SRC_LIST
    "docs/*.h"
    "*.c" "*.h"
    "init/*.c" "init/*.h"
    "cbtree/*.c" "cbtree/*.h"
    "cbtree/kmap/*.c" "cbtree/kmap/*.h"
    "callbacks/*.c" "callbacks/*.h"
)

set(BUILD_TYPE_DEBUG on)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
add_executable(${PROJECT_NAME} ${SRC_LIST})
