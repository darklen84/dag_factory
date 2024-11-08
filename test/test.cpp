#include <catch2/catch.hpp>

#include "dag/dag_factory.h"

using namespace dag;
struct Base {};

struct A : public Base {};
struct B : public Base {
  explicit B(A &a) {}
};
struct C : public Base {
  C(A &a, B &b) {}
};

struct D : public Base {
  D(B &b, C &c) {}
};

TEST_CASE(
    "factory functions tagged with DAG_SHARED() returns the same instance across multiple calls",
    "Blueprint") {
  struct System : public Blueprint<B> {
    A &a() { return dag->create<A>(); }
    B &b() DAG_SHARED(B) { return dag->create<B>(a()); }
    C &c() { return dag->create<C>(a(), b()); }
    D &d() { return dag->create<D>(b(), c()); }
    void config() { d(); }
  };

  auto dag = bootstrap<System>(std::mem_fn(&System::config));
  ;

  REQUIRE(dag->entryPoints().size() == 1);
}

TEST_CASE("normal factory function returns a new instance across multiple calls", "Blueprint") {
  struct System : public Blueprint<A> {
    A &a() { return dag->create<A>(); }
    B &b() DAG_SHARED(B) { return dag->create<B>(a()); }
    C &c() { return dag->create<C>(a(), b()); }
    D &d() { return dag->create<D>(b(), c()); }
    void config() { d(); }
  };

  auto dag = bootstrap<System>(std::mem_fn(&System::config));

  REQUIRE(dag->entryPoints().size() == 2);
}

TEST_CASE("factory can be overriden using runtime polymorphism", "Blueprint") {
  struct System : public Blueprint<B> {
    A &a() { return dag->create<A>(); }
    virtual B &b() DAG_SHARED(B) { return dag->create<B>(a()); }
    C &c() { return dag->create<C>(a(), b()); }
    D &d() { return dag->create<D>(b(), c()); }
    void config() { d(); }
  };

  struct System2 : public System {
    B &b() { return dag->create<B>(a()); }
  };

  auto dag = bootstrap<System2>(std::mem_fn(&System2::config));

  REQUIRE(dag->entryPoints().size() == 2);
}

template <typename Derived>
struct CRTPBase : public Blueprint<B> {
  Derived *derived = static_cast<Derived *>(this);

  A &a() { return dag->create<A>(); }
  B &b() DAG_SHARED(B) { return dag->create<B>(derived->a()); }
  C &c() { return dag->create<C>(derived->a(), derived->b()); }
  D &d() { return dag->create<D>(derived->b(), derived->c()); }
};

template <typename Derived>
struct CRTPSystem : public CRTPBase<Derived> {
  using Blueprint<B>::dag;
  Derived *derived = static_cast<Derived *>(this);
  B &b() { return dag->create<B>(derived->a()); }
};

struct CRTPSystem2 : public CRTPSystem<CRTPSystem2> {
  using Blueprint<B>::dag;
  void config() { d(); }
};

// namespace aaa
TEST_CASE("factory can be overriden using curiously recurring template", "Blueprint") {
  auto dag = bootstrap<CRTPSystem2>(std::mem_fn(&CRTPSystem2::config));

  REQUIRE(dag->entryPoints().size() == 2);
}