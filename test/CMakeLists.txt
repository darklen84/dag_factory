include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.0
)

FetchContent_MakeAvailable(Catch2)


add_executable(dag_factory_tests test.cpp)
target_link_libraries(dag_factory_tests PRIVATE 
    Catch2::Catch2WithMain
    dag_factory
)


add_test(NAME dag_factory_tests COMMAND dag_factory_tests)
