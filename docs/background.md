## Background

C++ developers are often divided on the topic of dependency injection (DI). The debate isn't just about which DI library to choose (also common in other languages) but also whether DI libraries are necessary in C++ at all. Many prefer to manually create the entire object graph by explicitly wiring dependencies, claiming this approach is simpler and more maintainable than using a DI framework. Is this because they have overlooked complexities of DI, or have existing DI frameworks strayed from what developers truly need?

Before try to answer these questions, let's examine the characteristics C++ developers expect from a DI library and the aspects they tend to avoid.

## Aspects Developers Like:
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

## Aspects Developers Dislike:
- **Learning a New DSL**: The need to learn a new domain-specific language can be a barrier.
- **Difficult Diagnosis**: Troubleshooting issues can be challenging.
- **Hidden Semantics**: Unexpected behaviors due to hidden semantics can cause surprises.

## The Answer

The manual dependency injection methods previously mentioned actually meet 7 out of 10 characteristics developers like and do not have any characteristics developers dislike. Their score is even higher than most of the existing DI frameworks. 

The preference for manual dependency injection is not without merit. It reflects the views of a significant portion of C++ developers who value simplicity and explicitness over a comprehensive feature set.

Instead of teaching C++ developers to adopt dependency injection methods and concepts borrowed from other languages, Dag_factory aims to enhance the existing dependency injection techniques that C++ developers are already familiar with and improve upon them.

Using the **factory** approach as the basis, Dag_factory introduces enhancements to achieve essential dependency injection features typically found in a DI framework.

Here is a brief list of the features Dag_factory provides:

- Classic interface based injection.
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
