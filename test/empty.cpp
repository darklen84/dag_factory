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

template <typename T> struct MySystem  {
  using EntryPoint = T;
  DagFactory<EntryPoint> *m_factory = nullptr;
   template<typename R, typename ... Args> R& create(Args &&... args) {
    return m_factory->template dedicated<R>(std::forward<Args>(args)...);
  }

   template<typename R, typename ... Args> R& create2(TYPE<R> t,Args &&... args) {
    return m_factory->template dedicated<R>(std::forward<Args>(args)...);
  }
  template <typename R>
  void test(const R* xxx){}
  //using Bluepoint<EntryPoint>::create;
  A &a() { test("aaa"); return create<A>(); }
  A& a2(){return create2(TYPE<A>());}
 /* B &b() { return m_factory->template shared<B>(a()); }
  C &c() { return m_factory->dedicated<C>(a(), b()); }
  D &d() { return m_factory->dedicated<D>(b(), c()); }*/
};

template <typename T> struct MySystem2: public MySystem<T>  {
  
  using MySystem<T>::create;
  using MySystem<T>::create2;
  B &b(){return this-> template create<B>();}
  B& b1(DagFactory<T>* factory) {
     this->test("aaa");
     return create2(TYPE<B>());
  }
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
