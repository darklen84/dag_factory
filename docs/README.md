Dag_factory is a single-header C++17 dependency injection (DI) library that employs a different approach compared to other DI libraries. 

Instead of attempting to imitate well-known DI libraries from other languages and striving to create a similar domain-specific language (DSL) through template metaprogramming—which often results in overly complicated and difficult-to-understand code, it enhances the existing manual dependency injection methods commonly used by developers, to make it more maintainable and more performant.

Before we explore what Dag_factory can do, let's have a brief tour of the two popular approaches c++ developer doing dependency injection without the help of DI libraries and discuss their strength and limitations.

And we want to create an object graph like below:
<!-- tabs:start -->

### **Graph**

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
### **Classes**

```cpp
struct B;

struct C;

struct A {
  explicit A(B& b1, B& b2) {}
};

struct B {
  explicit B(C& c) {}
};

struct C {};

```

<!-- tabs:end -->
Here is one approach people use ( we will called it **hard_wiring** approach in the rest of the document):

[snippet](snippets/hard_wiring.cpp ':include :type=code :fragment=hard_wiring')

Here is another approach ( we call it **factory** approach):

[snippet](snippets/factory.cpp ':include :type=code :fragment=factory')



>Note: You also needs to change the constructor parameters of the corresponding classes to be `std::unique_ptr` or `std::shared_ptr` as well.


Here is a quick comparasion of the two:

| Approach       | Pros                                                          | Cons                                                      |
|----------------|---------------------------------------------------------------|-----------------------------------------------------------|
| Hard Wiring    | Best performance and space locality.                          | Rigid, requires dependencies to be declared before their dependents. |
| Factory        | More flexible, dependencies can be overridden by subclassing. | Requires heap allocation, uses smart pointers.            |

While both approaches lack some functionality provided by many DI frameworks, they offer clean, efficient, and functional dependency injection that is widely understood by C++ developers.

At first glance, Dag_factory appears to be an enhanced version of the **factory** approach, achieving the same performance as the **hard_wiring** approach while maintaining its relative flexibility.

Here is how Dag_factory builds the same graph:

[snappit](snippets/dag_factory.cpp ':include :type=code :fragment=dag_factory_include')

[snappit](snippets/dag_factory.cpp ':include :type=code :fragment=dag_factory_factory_1')



Here is the step-by-step explanation:

* The blueprint derives from `dag::Blueprint<T>`, which provides helpers like `make_node<T>()`.
* `make_node<T>()` is similar to `std::make_unique` or `std::make_shared`, but the created object is owned by the graph, and a reference to the object is returned.
* `DAG_TEMPLATE_HELPER()` is optional; it provides syntactic sugar so you can call `make_node<T>(...)` directly instead of `this->template make_node<T>(...)`.
* The `dag_shared` macro acts as a method modifier, indicating that only a single instance of `C` exists within the graph, meaning all other nodes will reference this same instance. If you prefer all macros to be uppercase, `DAG_SHARED` is available with the same functionality.
* Dag_factory efficiently manages the lifecycle of all nodes within the graph. This allows nodes to receive their dependencies as references, ensuring that the dependencies are properly constructed and destructed in accordance with the graph's lifecycle. The entire graph is deleted after the `obj` variable goes out of scope.

Compared to the original **factory** approach, it eliminates the need to use smart pointers. Dag_factory can construct the entire graph on a given `std::pmr::memory_resource`. When passed an arena-style memory resource like `std::pmr::monotonic_buffer_resource`, the whole graph can be constructed in a contiguous memory block, achieving the same level of performance and data locality as the **hard_wiring** approach.

Here is exactly how:

[snappit](snippets/dag_factory.cpp ':include :type=code :fragment=dag_factory_factory_2')



