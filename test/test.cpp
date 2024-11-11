#include <catch2/catch.hpp>

#include "dag/dag_factory.h"

using namespace dag;
namespace {
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

template <typename T>
struct System : public Blueprint<T> {
  DAG_TEMPLATE_FACTORY()
  A &a() { return make_node<A>(); }
  virtual B &b() DAG_SHARED(B) { return make_node<B>(a()); }
  C &c() { return make_node<C>(a(), b()); }
  D &d() { return make_node<D>(b(), c()); }
  void config() { d(); }
};

}  // namespace
//------------------------------------------------------------------------------
TEST_CASE(
    "factory functions tagged with DAG_SHARED() returns the same instance across multiple calls",
    "Blueprint") {
  auto dag = bootstrap<System<B>>(std::mem_fn(&System<B>::config));
  ;

  REQUIRE(dag->entryPoints().size() == 1);
}

//------------------------------------------------------------------------------
TEST_CASE("normal factory function returns a new instance across multiple calls", "Blueprint") {
  auto dag = bootstrap<System<A>>(std::mem_fn(&System<A>::config));

  REQUIRE(dag->entryPoints().size() == 2);
}
//------------------------------------------------------------------------------
TEST_CASE("factory can be overriden using runtime polymorphism", "Blueprint") {
  struct System2 : public System<B> {
    B &b() override { return make_node<B>(a()); }
  };

  auto dag = bootstrap<System2>(std::mem_fn(&System2::config));

  REQUIRE(dag->entryPoints().size() == 2);
}
//------------------------------------------------------------------------------
namespace {
template <typename Derived>
struct CRTPBase : public Blueprint<B> {
  DAG_TEMPLATE_FACTORY()
  Derived *derived = static_cast<Derived *>(this);

  A &a() { return make_node<A>(); }
  B &b() DAG_SHARED(B) { return make_node<B>(derived->a()); }
  C &c() { return make_node<C>(derived->a(), derived->b()); }
  D &d() { return make_node<D>(derived->b(), derived->c()); }
};

template <typename Derived>
struct CRTPSystem : public CRTPBase<Derived> {
  DAG_TEMPLATE_FACTORY()
  Derived *derived = static_cast<Derived *>(this);
  B &b() { return make_node<B>(derived->a()); }
};

struct CRTPSystem2 : public CRTPSystem<CRTPSystem2> {
  using Blueprint<B>::_hidden_state;
  void config() { d(); }
};
}  // namespace

TEST_CASE("factory can be overriden using curiously recurring template", "Blueprint") {
  auto dag = bootstrap<CRTPSystem2>(std::mem_fn(&CRTPSystem2::config));

  REQUIRE(dag->entryPoints().size() == 2);
}

struct System : public Blueprint<B> {
  A &a() { return make_node<A>(); }
  B &b() DAG_SHARED(B) { return make_node<B>(a()); }
  C &c() { return make_node<C>(a(), b()); }
  D &d() { return make_node<D>(b(), c()); }
  void config() { d(); }
};