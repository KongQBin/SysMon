project(SysMon)
include_directories(
    ./
    ./cbtree
    ./cbtree/kmap
    ./callbacks
    ./init
    /usr/include
    /usr/local/include
#    /opt/gcc-9.5.0/include
#    /opt/gcc-9.5.0/include/c++/9.5.0
    )

cmake_minimum_required(VERSION 2.8)
aux_source_directory(. SRC_LIST)
aux_source_directory(./init SRC_LIST)
aux_source_directory(./cbtree SRC_LIST)
aux_source_directory(./cbtree/kmap SRC_LIST)
aux_source_directory(./callbacks SRC_LIST)
# link_libraries(
#     /lib64/libselinux.so
#     # /lib64/libseccomp.so.2
# )
set(BUILD_TYPE_DEBUG on)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
add_executable(${PROJECT_NAME} ${SRC_LIST}
    ./cbtree/cbtree.h
    ./cbtree/kmap/rbtree.h
    ./init/init.h
    ./callbacks/callbacks.h
    ./callbacks/write.h
    )
