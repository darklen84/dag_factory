#include "common.hpp"

namespace {
/// [hard_wiring]
struct Container {
  C c;
  B b1{c};
  B b2{c};
  A a{b1, b2};
};

static void test() {
  Container container;
  A& obj = container.a;
}
/// [hard_wiring]
}  // namespace
