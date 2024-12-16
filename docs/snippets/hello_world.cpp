#include <iostream>
#include <ostream>

#include "dag/dag_factory.h"

template <typename T>
struct SanityBlueprint : public dag::Blueprint<T> {
  DAG_TEMPLATE_HELPER();
  std::string &helloworld() { return make_node<std::string>("Hello World!"); }
};

int main() {
  auto factory = dag::DagFactory<SanityBlueprint>{};
  dag::unique_ptr<std::string> greet =
      factory.create([](auto bp) -> auto & { return bp->helloworld(); });
  std::cout << *greet << std::endl;
}