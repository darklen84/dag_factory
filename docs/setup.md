## Prerequisite

All you need is a C++ compiler that support C++17.

## Setup

Dag_factory is a single header library that can be downloaded directly from the browser [here](
https://raw.githubusercontent.com/darklen84/dag_factory/refs/tags/v0.1.0/include/dag/dag_factory.h), or via the console:

```
wget https://raw.githubusercontent.com/darklen84/dag_factory/refs/tags/v0.1.0/include/dag/dag_factory.h
```

Simply copy it to a directory named `dag` under your include directory so that the it can be included like follow:

```cpp
#include "dag/dag_factory.h"
```

## Setup with CMake

It can also integrated into an existing CMake project using `FetchContent`.

```
include(FetchContent)

FetchContent_Declare(
    dag_factory
    GIT_REPOSITORY https://github.com/darklen84/dag_factory.git
    GIT_TAG v0.1.0
)

set(DAG_ENABLE_TEST OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(dag_factory)

add_executable(dag_test test.cpp)
target_link_libraries(dag_test PRIVATE dag_factory::Core)
```

## Sanity check

Run following snippet to verify Dag_factory is setup correctly:

[snappit](snippets/hello_world.cpp ':include :type=code')

You are good to go if the snippet builds and produces following output at runtime:

```
Hello World!
```


