## Introduction

Dag_factory is a single-header C++17 dependency injection (DI) library that employs a different approach compared to other DI libraries. 

Instead of attempting to imitate well-known DI libraries from other languages and striving to create a similar domain-specific language (DSL) through template metaprogramming—which often results in overly complicated and difficult-to-understand code, it enhances the existing manual dependency injection methods commonly used by developers, to make it more maintainable and more performant.

Before we explore what Dag_factory can do, let's have a brief tour of the two popular approaches c++ developer doing dependency injection without the help of DI libraries and discuss their strength and limitations.

Assume that we have following classes:
```c++
struct A {
    explicit A(B& b1, B& b2) {}  
};

struct B {
    explicit B(C& c) {}
};

struct C {};

```
And we want to create an object graph like below:

```plantuml
object "a:A" as a
object "b1:B" as b_1
object "b2:B" as b_2
object "c:C" as c

a --> b_1
a --> b_2
b_1 --> c
b_2 --> c
```

Here is one approach people use ( we will called it **hard_wiring** approach in the rest of the document):

```C++
struct Container{
    C c;
    B b1{c};
    B b2{c};
    A a{b1, b2};
};

void test() {
    Container container;
    A& obj = container.a;
}
```
Here is another approach ( we call it **factory** approach):

```C++
struct Factory {
    std::unique_ptr<A> a() { return std::make_unique<A>(b(), b()); }
    std::unique_ptr<B> b() { return std::make_unique<B>(c());}

    std::shared_ptr<C> c_ptr;
    std::shared_ptr<C> c() {
        if(nullptr == c_ptr){
            c_ptr = std::make_shared<C>();
        }
        return c_ptr;
    }
};

void test() {
    Factory factory;
    unique_ptr<A> obj = factory.a();
}

```
Here is a quick comparasion of the two:

| Approach       | Pros                                                          | Cons                                                      |
|----------------|---------------------------------------------------------------|-----------------------------------------------------------|
| Hard Wiring    | Best performance and space locality.                          | Rigid, requires dependencies to be declared before their dependents. |
| Factory        | More flexible, dependencies can be overridden by subclassing. | Requires heap allocation, uses smart pointers.            |

While both approaches lack some functionality provided by many DI frameworks, they offer clean, efficient, and functional dependency injection that is widely understood by C++ developers.

At first glance, Dag_factory appears to be an enhanced version of the **factory** approach, achieving the same performance as the **hard_wiring** approach while maintaining its relative flexibility.

Here is how Dag_factory builds the same graph:

```c++
#include "dag/dag_factory.h"

template<typename T>
struct SystemBlueprint : public dag::Blueprint<T> {
    DAG_TEMPLATE_HELPER();
    A& a() { return make_node<A>(b(), b()); }
    B& b() { return make_node<B>(c()); }
    C& c() dag_shared { return make_node<C>(); }
};

void test() {
    auto factory = DagFactory<SystemBlueprint>();
    dag::unique_ptr<A> obj = factory.create([](auto bp) -> auto& { return bp->a(); });
}
```

Here is the step-by-step explanation:

* The blueprint derives from `dag::Blueprint<T>`, which provides helpers like `make_node<T>()`.
* `make_node<T>()` is similar to `std::make_unique` or `std::make_shared`, but the created object is owned by the graph, and a reference to the object is returned.
* `DAG_TEMPLATE_HELPER()` is optional; it provides syntactic sugar so you can call `make_node<T>(...)` directly instead of `this->template make_node<T>(...)`.
* The `dag_shared` macro acts as a method modifier, indicating that only a single instance of `C` exists within the graph, meaning all other nodes will reference this same instance. If you prefer all macros to be uppercase, `DAG_SHARED` is available with the same functionality.
* Dag_factory efficiently manages the lifecycle of all nodes within the graph. This allows nodes to receive their dependencies as references, ensuring that the dependencies are properly constructed and destructed in accordance with the graph's lifecycle. The entire graph is deleted after the `obj` variable goes out of scope.

Compared to the original **factory** approach, it eliminates the need to use smart pointers. Dag_factory can construct the entire graph on a given `std::pmr::memory_resource`. When passed an arena-style memory resource like `std::pmr::monotonic_buffer_resource`, the whole graph can be constructed in a contiguous memory block, achieving the same level of performance and data locality as the **hard_wiring** approach.

Here is exactly how:

```c++
void test() {
    // Pre-allocate 1K memory
    std::pmr::monotonic_buffer_resource memory(1024);
    auto factory = DagFactory<SystemBlueprint>(&memory);
    dag::unique_ptr<A> obj = factory.create([](auto bp) -> auto& { return bp->a(); });
}
```


# Background

C++ developers are often divided on the topic of dependency injection (DI). The debate isn't just about which DI library to choose (also common in other languages) but also whether DI libraries are necessary in C++ at all. Many prefer to manually create the entire object graph by explicitly wiring dependencies, claiming this approach is simpler and more maintainable than using a DI framework. Is this because they have overlooked complexities of DI, or have existing DI frameworks strayed from what developers truly need?

Before try to answer these questions, let's examine the characteristics C++ developers expect from a DI library and the aspects they tend to avoid.

## What Developers Like:
- **Nonintrusive**: Seamlessly integrates without altering existing code.
- **Compile-time Validation**: Ensures errors are caught early, making the code robust.
- **Zero Overhead**: Don't pay the overhead from the features you don't use. (e.g., thread safety).
- **High Performance**: Avoids unnecessary performance costs, no forced heap allocation or `shared_ptr` usage.
- **Zero Third-party Dependencies**: Pure C++ solution without external dependencies.
- **Support for Multiple Paradigms**: Accommodates both object-oriented and template-based injection.
- **Transparency**: Dependency resolution is clear and understandable.
- **Flexibility**: Easily update bindings as needed.
- **Ease of Integration**: Simple to incorporate into existing projects.
- **Intelligent**: Automatically wires dependencies through compile-time inference.

## What Developers Dislike:
- **Learning a New DSL**: The need to learn a new domain-specific language can be a barrier.
- **Difficult Diagnosis**: Troubleshooting issues can be challenging.
- **Hidden Semantics**: Unexpected behaviors due to hidden semantics can cause surprises.

## The Observation
The manual dependency injection methods previously mentioned actually meet 7 out of 10 characteristics developers like and do not have any characteristics developers dislike. Their score is even higher than most of the existing DI frameworks. 

The preference for manual dependency injection is not without merit. It reflects the views of a significant portion of C++ developers who value simplicity and explicitness over a comprehensive feature set.

Instead of teaching C++ developers to adopt dependency injection methods and concepts borrowed from other languages, Dag_factory aims to enhance the existing dependency injection techniques that C++ developers are already familiar with and improve upon them.

Using the **factory** approach as the basis, Dag_factory introduces enhancements to achieve features typically found in a DI framework. As a principle, Dag_factory only adds features that can be achieved without touching any areas that developers dislike.

Here is a brief list of the features Dag_factory provides:

- Classic virtual function-based injection.
- Template-based injection.
- Scoping support, allowing a single instance of a given class to be shared within a scope.
- Nested injection.
- Flexible mapping of interfaces to concrete classes.
- Thread safety.
- Built-in efficient object lifecycle management.
- Support for user-provided extensions to intercept object creation.
- Collection of all objects that are or are derived from a given type.

One notable feature supported by other DI frameworks but not by Dag_factory is auto-wiring, which is the ability to infer and create dependencies automatically. Dag_factory does not support this because it cannot be implemented without becoming intrusive or introducing hidden semantics through heavy template metaprogramming. However, even without auto-wiring, managing dependencies explicitly within a blueprint (a class that defines the dependencies of a graph) is still manageable, even for larger projects containing hundreds of classes. When dependencies change, only two places need to be updated: the constructor of the class and the blueprint. In contrast, with auto-wiring, only the constructor of the class would need to be changed. While explicitly specifying object dependencies in a centralized blueprint may seem like a limitation, it actually aids in understanding and maintaining large codebases. If you value explicitness and are willing to take this small extra step, Dag_factory is the right choice for you.

If auto-wiring is a crucial feature for your project, Dag_factory may not be the best fit. Consider exploring other C++ DI libraries that offer auto-wiring capabilities.

