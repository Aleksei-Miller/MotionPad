cmake_minimum_required(VERSION 3.15)
project(motionpad_tests C)

set(CMAKE_C_STANDARD 99)

add_executable(motionpad_tests 
    test_input_actions.c
)
