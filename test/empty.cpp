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
struct MySystem : public Bluepoint<A> {
  A &a() { return m_factory->dedicated<A>(); }
  B &b() { return m_factory->shared<B>(a()); }
  C &c() { return m_factory->dedicated<C>(a(), b()); }
  D &d() { return m_factory->dedicated<D>(b(), c()); }
};

TEST_CASE("test", "test") {
  std::unique_ptr<Dag<A>> dag =
      bootstrap<MySystem>([](MySystem &system) { system.d(); });
  REQUIRE(dag->entryPoints().size() == 2);
  /*Dag<void> dag;
  DagFactory factory{dag};
  std::string &s = factory.dedicated<std::string>("aaa");
  std::cout << s << std::endl;*/
}
