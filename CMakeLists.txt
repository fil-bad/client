cmake_minimum_required(VERSION 3.9)
project(client C)

set(CMAKE_C_STANDARD 11)

find_package(Threads)

set(SOURCE src/client.c src/mexData.c src/tableFile.c src/fileSystemUtylity.c )

set(HEADER include/client.h include/mexData.h include/tableFile.h include/fileSystemUtylity.h)

file(GLOB HEADER_TREE treeFunx/include/*)
file(GLOB SOURCE_TREE treeFunx/src/*)


add_executable(client main.c globalSet.h ${HEADER} ${SOURCE} ${HEADER_TREE} ${SOURCE_TREE})

target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
