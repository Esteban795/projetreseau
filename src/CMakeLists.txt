cmake_minimum_required(VERSION 3.10)

project(projetreseau)

add_executable(projetreseau main.c)

add_subdirectory(scheduler)
add_subdirectory(agent)

target_link_libraries(projetreseau scheduler)
target_link_libraries(projetreseau agent)