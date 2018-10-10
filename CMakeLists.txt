cmake_minimum_required(VERSION 3.9)
project(client C)

set(CMAKE_C_STANDARD 11)

find_package(Threads)

add_executable(client main.c client.c client.h)

target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
