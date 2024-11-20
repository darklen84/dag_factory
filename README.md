[![CMake on multiple platforms](https://github.com/darklen84/dag_factory/actions/workflows/cmake-multi-platform.yml/badge.svg)](https://github.com/darklen84/dag_factory/actions/workflows/cmake-multi-platform.yml)
[![License](https://img.shields.io/badge/License-BSD_2--Clause-orange.svg)](https://opensource.org/licenses/BSD-2-Clause)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=darklen84_dag_factory&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=darklen84_dag_factory)

# Background

C++ developers are often divided on the topic of dependency injection (DI). The debate isn't just about which DI library to choose (also common in other languages) but also whether DI libraries are necessary in C++ at all. Many prefer to manually create the entire object graph by explicitly wiring dependencies, claiming this approach is simpler and more maintainable than using a DI framework. Is this because they are unfamiliar with the complexities of DI, or have existing DI frameworks strayed from what developers truly need?

To answer these questions, let's examine the characteristics C++ developers expect from a DI library and the aspects they tend to avoid.

## What Developers Like:
- **Nonintrusive**: Seamlessly integrates without altering existing code.
- **Compile-time Validation**: Ensures errors are caught early, making the code robust.
- **Comprehensive Feature Set**: Offers extensive features without imposing overhead unless used (e.g., thread safety).
- **Minimal Overhead**: Avoids unnecessary performance costs, no forced heap allocation or `shared_ptr` usage.
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

## DI without framework

We also discuss common methods C++ developers use to perform dependency injection manually.

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

![](https://www.plantuml.com/plantuml/svg/SoWkIImgAStDuSfFoafDBb5GIhHoL598B5P8X8ia6LevWOMIuWqHWaPmGIEuOBALCrWicOihKK5Nrmwa0y82AmFoG6oWD907PJcavgK0hGS0)

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
While both approaches have their limitations, the **hard_wiring** approach might seem rigid—requiring dependencies to be declared before their dependents, leading to maintenance challenges as the number of objects increases. The **factory** approach, on the other hand, requires the use of smart pointers and enforces heap allocation.

Yet, let's not overlook their strengths! The hard_wiring approach offers the best performance and space locality we can theoretically achieve when creating the desired graph. Both approaches are clean and straightforward.

When we compare them with the checklist above, we find they meet 6 out of the 10 aspects developers like and amazingly don't hit any aspects developers dislike! That score is even better than many of the C++ DI frameworks out there!

# dag_factory

Dag_factory is a single-header C++17 dependency injection (DI) library that employs a different approach compared to other DI libraries. 

Instead of attempting to imitate well-known DI libraries from other languages and striving to create a similar domain-specific language (DSL) through template metaprogramming—which often results in overly complicated and difficult-to-understand code—it enhances the existing manual dependency injection methods commonly used by developers, particularly the **factory** approach we mentioned previously. With this enhancement, the **factory** approach can achieve the performance of the *hard_wiring* approach while offering flexibility similar to that provided by other DI frameworks.

Dag_factory uses plain C++ templates to model the dependency information in the object graph, called a blueprint. This blueprint is similar to the factory class in the **factory** approach but offers improved performance and usability. Below is an example of a simple blueprint:

```c++
#include "dag/dag_factory.h"

template<typename T>
struct SystemBp: public dag::Blueprint<T> {
    DAG_TEMPLATE_HELPER();
    A& a() { return make_node<A>(b(), b()); }
    B& b() { return make_node<B>(c()); }
    C& c() dag_shared { return make_node<C>(); }
};

void test() {
    auto factory = DagFactory<SystemBlueprint>();
    dag::unique_ptr<A> obj = factory.create([](auto bp) -> auto & { return bp->a(); }); 
}
```
Here is the step-by-step explanation:

* The blueprint derives from `dag::Blueprint<T>`, which provides utility methods like `make_node<T>()`.
* `make_node<T>()` is similar to `std::make_unique` or `std::make_shared`, but the created object is owned by the graph, and a reference to the object is returned.
* `DAG_TEMPLATE_HELPER()` is optional; it provides syntactic sugar so you can call `make_node<T>(...)` directly instead of `this->template make_node<T>(...)`.
* The `dag_shared` macro acts as a method modifier, indicating that only a single instance of `C` exists within the graph, meaning all other nodes will reference this same instance. If you prefer all macros to be uppercase, `DAG_SHARED` is available with the same functionality.
* The entire graph is deleted after the `obj` variable goes out of scope. `dag_factory` efficiently manages the lifecycle of all nodes within the graph. This allows nodes to receive their dependencies as references, ensuring that the dependencies are properly constructed and destructed in accordance with the graph's lifecycle.

Compared to the original **factory** approach, it eliminates the need to use smart pointers. `dag_factory` can construct the entire graph on a given `std::pmr::memory_resource`. When passed an arena-style memory resource like `std::pmr::monotonic_buffer_resource`, the whole graph can be constructed in a contiguous memory block, achieving the same level of performance and data locality as the **hard_wiring** approach.

Here is how:
```

```
void test() {
    //pre-alocate 1K memory
    std::pmr::memory_resource memory(1024);
    auto factory = DagFactory<SystemBlueprint>(&memory);
    dag::unique_ptr<A> obj = factory.create([](auto bp) -> auto & { return bp->a(); }); 
}
```

(end)

Creating a dependency injection framework in for that fits for all is difficult, due to many reasons:
On one hand, C++'s omit of runtime reflection, make it more difffult to create a depdnencies injection framwork that has the same feature set as the famous injection frameworks in other languages. C++'s powerful compile time computation capability somewhat mitigates this limitation but it also result on highly complicated templatized code that is diffult to understand and debug.

Depndency injection in c++ is aslo  more complex problem to solve than other languages. Without garbage collection, an injection framework in C++ have to not only handles the objects dependency but also their life cycle and ownership. With more programming paradims in C++, the classic dependnecy injection which based on runtime polymorphism, is not the only, and even not the prefered choice to do dependency injection. Template base dependency are prefered by many C++ developers, which impose unique challenges and complexities to 

and garbage collection, without them, Most existing C++ dependency injection frameworks tends to use heavily templatized code and deal with the object dependency as well as their lifecircles. 

Dag_factory is a single-header dependency injection framework for C++ 17, enabling the creation of a graph of interconnected objects in a single operation.

Dependency injection is a design pattern that decouples object creation from their dependencies, leading to more modular, testable, and maintainable code. By using dependency injection, developers can easily swap out implementations, mock dependencies for testing, and manage complex object graphs with ease.

Existing C++ dependency injection frameworks often come with limitations such as overly complex configuration, intrusiveness, and performance overhead. Dag_factory addresses these issues by providing a lightweight, header-only solution that leverages the latest C++ features to offer efficient and straightforward dependency injection.

Dag_factory uses a plain C++ template to model the dependency information in the object graph, referred to as a blueprint. Below is an example of a simple blueprint:

```c++
#include "dag/dag_factory.h"
template<typename T>
struct SystemBlueprint : public dag::Blueprint<T> {
    A& a() { return make_node<A>(b(), b()); }
    B& b() { return make_node<B>(c()); }
    C& c() dag_shared { return make_node<C>(); }
};
```
The `make_node()` template method functions similarly to `std::make_unique`, but the created object is owned by the graph and it returns a reference. The `dag_shared` macro acts as a method modifier, indicating that only a single instance of `C` exists within the graph, meaning all other nodes will reference this same instance. If you prefer all macros to be uppercase, `DAG_SHARED` is available with the same functionality.


This defines a graph of four objects as shown below:

![](https://www.plantuml.com/plantuml/png/SoWkIImgAStDuSfFoafDBb5GIhHoL598B5P8X8ia6LevWOMIuWqHWaPmGIEuOBALCrWicOihKK5Nrmwa0y82AmFoG6oWD907PJcavgK0hGS0)


The following code construct the object graph and returns one of its node `a`:

```c++
auto factory = DagFactory<SystemBlueprint>();

dag::unique_ptr<A> obj = factory.create(std::mem_fn(&SystemBlueprint::a));
```

The entire graph is deleted after the `obj` variable goes out of scope.

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






