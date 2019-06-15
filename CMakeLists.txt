cmake_minimum_required(VERSION 3.13)
project(HW4 C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(HW4
        core_api.c
        core_api.h
        main.c
        sim_api.h
        sim_mem.c)
