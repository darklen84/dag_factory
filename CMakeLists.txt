cmake_minimum_required(VERSION 3.15)
project(dag_factory)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_STANDARD_REQUIRED True)

enable_testing()

add_library(dag_factory INTERFACE)

target_include_directories(dag_factory INTERFACE include)

install(TARGETS dag_factory
        EXPORT dag_factoryTargets
        INCLUDES DESTINATION include
        )

install(DIRECTORY include/ DESTINATION include)

add_subdirectory(test)

