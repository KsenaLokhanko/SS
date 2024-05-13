cmake_minimum_required(VERSION 3.27)
project(labW2)

set(CMAKE_CXX_STANDARD 17)

add_executable(labW2 main.c
        kernel.c
        "tester (4).c"
        slab.c)
