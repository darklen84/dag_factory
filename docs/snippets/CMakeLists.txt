add_executable(doc_snippets
    main.cpp
    hard_wiring.cpp
    factory.cpp
    dag_factory.cpp
)

target_link_libraries(doc_snippets PRIVATE dag_factory)