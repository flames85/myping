cmake_minimum_required(VERSION 2.8)
project(myping)

set(SOURCE_FILES main.c myping.c myping.h)
add_executable(myping ${SOURCE_FILES})
