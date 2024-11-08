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

template<typename T>
struct TYPE {
};
template<typename Derived>
struct MySystemBase:public Bluepoint<A>  {
  Derived* self = static_cast<Derived*>(this);
  A &a() { return dag->dedicated<A>();}
  B &b() { return dag->template shared<B>(self->a()); }
  C &c() { return dag->dedicated<C>(self->a(), self->b()); }
  D &d() { return dag->dedicated<D>(self->b(), self->c()); }
};
template<typename Derived>
struct MySystem0: public MySystemBase<Derived> {
  using Bluepoint<A>::dag;
   Derived* self = static_cast<Derived*>(this);
    C &c() { return dag->dedicated<C>(self->a(), self->b()); }

};

struct MySystem: public MySystem0<MySystem>  {
  
  B &b(){return dag->dedicated<B>(a());}
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
      bootstrap<MySystem>([](MySystem *system) { /*system.d();*/ });
  REQUIRE(dag->entryPoints().size() == 2);
}
