project(SysMon)
cmake_minimum_required(VERSION 2.9)
include_directories(
    ./
    ./cbtree
    ./cbtree/kmap
    ./callbacks
    ./callbacks/comm
    ./general
    ./init
    /usr/include
    /usr/local/include
    )

FILE(GLOB MAIN_LIST
    "docs/*.h"
    "*.c" "*.h"
    "init/*.c" "init/*.h"
    "cbtree/*.c" "cbtree/*.h"
    "cbtree/kmap/*.c" "cbtree/kmap/*.h"
    "callbacks/*.c" "callbacks/*.h"
    "callbacks/comm/*.c" "callbacks/comm/*.h"
    "general/*.c" "general/*.h"
)

FILE(GLOB TEST_LIST
    "build/*.c"
)

set(BUILD_TYPE_DEBUG on)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
add_executable(test ${TEST_LIST})
add_executable(${PROJECT_NAME} ${MAIN_LIST})

link_libraries(pthread)
