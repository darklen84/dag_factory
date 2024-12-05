/// [dag_factory_include]
#include "dag/dag_factory.h"
/// [dag_factory_include]

#include "common.hpp"
/// [dag_factory_factory_1]
template <typename T>
struct SystemBlueprint : public dag::Blueprint<T> {
  DAG_TEMPLATE_HELPER();
  A& a() { return make_node<A>(b(), b()); }
  B& b() { return make_node<B>(c()); }
  C& c() dag_shared { return make_node<C>(); }
};

static void test() {
  auto factory = dag::DagFactory<SystemBlueprint>();
  dag::unique_ptr<A> obj = factory.create([](auto bp) -> auto& { return bp->a(); });
}
/// [dag_factory_factory_1]
/// [dag_factory_factory_2]
static void test2() {
  // Pre-allocate 1K memory
  std::pmr::monotonic_buffer_resource memory(1024);
  auto factory = dag::DagFactory<SystemBlueprint>(&memory);
  dag::unique_ptr<A> obj = factory.create([](auto bp) -> auto& { return bp->a(); });
}
/// [dag_factory_factory_2]
