cmake_minimum_required(VERSION 3.22.1)
project(ib_inspector C)

add_library(ib_inspector SHARED inspector.c)
target_compile_features(ib_inspector PRIVATE c_std_11)
target_compile_options(ib_inspector PRIVATE -Wall -Wextra -Wpedantic)
target_link_options(ib_inspector PRIVATE -u ANativeActivity_onCreate)
target_link_libraries(ib_inspector android log)
