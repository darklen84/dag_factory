cmake_minimum_required(VERSION 3.15)
project(dag_factory)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_STANDARD_REQUIRED True)

add_library(dag_factory INTERFACE)

target_include_directories(dag_factory INTERFACE include)

add_subdirectory(test)

