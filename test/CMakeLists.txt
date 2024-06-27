include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v2.13.4
)

FetchContent_Populate(Catch2)
add_subdirectory(${catch2_SOURCE_DIR} ${catch2_BINARY_DIR})

add_executable(dag_factory_tests empty.cpp)
target_link_libraries(dag_factory_tests PRIVATE Catch2::Catch2WithMain)