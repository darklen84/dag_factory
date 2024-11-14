# dag_factory
Dag_factory is a single-header dependency injection framework for C++. It supports C++17 and onward, enabling the creation of a graph of interconnected objects in a single operation.

Dependency injection is a design pattern that decouples object creation from their dependencies, leading to more modular, testable, and maintainable code. By using dependency injection, developers can easily swap out implementations, mock dependencies for testing, and manage complex object graphs with ease.

Existing C++ dependency injection frameworks often come with limitations such as overly complex configuration, intrusiveness, and performance overhead. Dag_factory addresses these issues by providing a lightweight, header-only solution that leverages the latest C++ features to offer efficient and straightforward dependency injection.

Dag_factory uses a plain C++ class to model the dependency information in the object graph, referred to as a blueprint. Below is an example of a simple blueprint:

```c++
#include "dag/dag_factory.h"

struct SystemBlueprint : public dag::Blueprint<SystemBlueprint> {
    A& a() { return make_node<A>(b(), b()); }
    B& b() { return make_node<B>(c()); }
    C& c() DAG_SHARED(C) { return make_node<C>(); }
};
```
The `make_node()` template method acted like `std::make_unique` excepted that the created object is owned by the graph and it returns a reference. The `DAG_SHARED()` macro ensures that only a single instance of `C` exists within the graph, meaning all other nodes will reference this same instance.


This defines a graph of four objects as shown below:

![](https://www.plantuml.com/plantuml/png/SoWkIImgAStDuSfFoafDBb5GIhHoL598B5P8X8ia6LevWOMIuWqHWaPmGIEuOBALCrWicOihKK5Nrmwa0y82AmFoG6oWD907PJcavgK0hGS0)


The following code construct the object graph and returns one of its node `a`:

```c++
auto factory = DagFactory<SystemBlueprint>();

// The variable root has the type of dag::unique_ptr<A>
auto [root, ignored] = factory.create(std::mem_fn(&SystemBlueprint::a));
```

The entire graph is deleted after the `root` variable goes out of scope.

Here are the declarations of all the classes involved:

```c++
struct A {
    explicit A(B& b1, B& b2) {}
};

struct B {
    explicit B(C& c) {}
};

struct C {};
```

Dag_factory efficiently manages the lifecycle of all nodes within the graph. This allows nodes to receive their dependencies as references, ensuring that the dependencies are properly constructed and destructed in accordance with the graph's lifecycle.






