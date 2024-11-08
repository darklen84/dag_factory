#include "dag/dag_factory.h"
#include <catch2/catch.hpp>
#include <iostream>
#include <ostream>
#include <string>

using namespace dag;
struct A {};
struct B {
  explicit B(A &a) {}
};
struct C {
  C(A &a, B &b) {}
};

struct D {
  D(B &b, C &c) {}
};
template <typename EntryPoint> struct MySystem : public Bluepoint<EntryPoint> {
  //using Bluepoint<EntryPoint>::create;
  A &a() { return this->template create<A>(); }
  B &b() { return m_factory->shared<B>(a()); }
  C &c() { return m_factory->dedicated<C>(a(), b()); }
  D &d() { return m_factory->dedicated<D>(b(), c()); }
};

template <typename T> struct Base {
  void xxx() {}
};

template <typename T> class Derived : public Base<T> {
  using Base<T>::xxx;
  void yyy() { xxx(); }
};

#define NAME(n, id) n##id
#define _SHARED(v, line)                                                       \
  v & { return *NAME(factory, line)(); }                                       \
  v *NAME(factory, line)()

#define DAG_SHARED(v) _SHARED(v, __LINE__)

struct XXX {
  auto a() -> DAG_SHARED(A) { return new A(); }
  auto b() -> DAG_SHARED(A) { return new A(); }
  auto c() -> DAG_SHARED(B) { return new B(a()); }
};

TEST_CASE("test", "test") {
  std::unique_ptr<Dag<A>> dag =
      bootstrap<MySystem<A>>([](MySystem<A> &system) { /*system.d();*/ });
  REQUIRE(dag->entryPoints().size() == 2);
}
