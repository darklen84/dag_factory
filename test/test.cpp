#include <catch2/catch_test_macros.hpp>
#include <map>

#include "dag/dag_factory.h"

using namespace dag;
namespace {
struct Base {};

struct A : public Base {
  A() = default;
  A(const A &a) = delete;
};
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
  DAG_TEMPLATE_HELPER()
  A &a() { return make_node<A>(); }
  virtual B &b() dag_shared { return make_node<B>(a()); }
  C &c() { return make_node<C>(a(), b()); }
  D &d() { return make_node<D>(b(), c()); }
  // D &config() { return d(); }
};

}  // namespace
//------------------------------------------------------------------------------
TEST_CASE(
    "factory functions tagged with DAG_SHARED() returns the same instance across multiple calls",
    "Blueprint") {
  auto factory = DagFactory<System, B>();
  auto [entry, selections] = factory.create([](auto bp) -> auto & { return bp->d(); });

  REQUIRE(selections->size() == 1);
}

//------------------------------------------------------------------------------
TEST_CASE("normal factory function returns a new instance across multiple calls", "Blueprint") {
  auto factory = DagFactory<System, A>();
  auto [entry, selections] = factory.create([](auto bp) -> auto & { return bp->d(); });

  REQUIRE(selections->size() == 2);
}

namespace {
template <typename T>
struct System2 : public System<T> {
  DAG_TEMPLATE_HELPER();
  using System<T>::a;
  B &b() override { return make_node<B>(a()); }
};
}  // namespace
//------------------------------------------------------------------------------
TEST_CASE("factory can be overriden using runtime polymorphism", "Blueprint") {
  auto factory = DagFactory<System, A>();
  auto [entry, selections] = factory.create([](auto bp) -> auto & { return bp->d(); });
  REQUIRE(selections->size() == 2);
}

TEST_CASE("TypeToCollect in Blueprint is optional", "Blueprint") {
  auto factory = DagFactory<System>();
  auto [entry, selections] = factory.create([](auto bp) -> auto & { return bp->d(); });
  REQUIRE(selections->size() == 0);
}
/*
TEST_CASE("BootStrapper::load() can return node directly", "Blueprint") {
  struct System2 : public System<B> {
    B &b() override { return make_node<B>(a()); }
  };
  auto [r, selections] = DagFactory<System2>().create(std::mem_fn(&System2::b));
}

TEST_CASE("DAG_SHARE() accepts types with comma", "Blueprint") {
  struct System : public Blueprint<std::map<int, int>> {
    std::map<int, int> &a() DAG_SHARED(std::map<int, int>) {
      return make_node<std::map<int, int>>();
    }
    std::map<int, int> &config() { return a(); }
  };
  auto [entry, selections] = DagFactory<System>().create(std::mem_fn(&System::config));

  REQUIRE(selections->size() == 1);
}
*/

namespace {
template <typename T>
struct System3 : public Blueprint<T> {
  DAG_TEMPLATE_HELPER();
  explicit System3(int k, int v) : key(k), value(v) {}
  int key;
  int value;
  std::map<int, int> &a() dag_shared {
    auto &map = make_node<std::map<int, int>>();
    map[key] = value;
    return map;
  }
  std::map<int, int> &config() { return a(); }
};
}  // namespace

TEST_CASE("Blueprints with custom constructor are supported", "Blueprint") {
  auto factory = DagFactory<System3, std::map<int, int>>();
  auto [entry, selections] = factory.create([](auto bp) -> auto & { return bp->a(); }, 1, 2);

  REQUIRE(selections->size() == 1);
  REQUIRE((*selections)[0]->operator[](1) == 2);
}

namespace {
template <typename T>
struct System4 : public Blueprint<T> {
  DAG_TEMPLATE_HELPER();
  std::pmr::vector<std::pmr::string> &a() {
    std::pmr::vector<std::pmr::string> &v = make_node<std::pmr::vector<std::pmr::string>>();
    v.push_back("a");
    return v;
  }
};
}  // namespace

TEST_CASE("factory use and propagate the memory resource", "Resource") {
  auto memory = std::pmr::new_delete_resource();
  auto factory = DagFactory<System4, std::pmr::vector<std::pmr::string>>(memory);
  auto [entry, selections] = factory.create([](auto bp) -> auto & { return bp->a(); });

  REQUIRE(selections->size() == 1);

  REQUIRE((*selections)[0]->get_allocator().resource() == memory);
  REQUIRE((*selections)[0][0].get_allocator().resource() == memory);
  REQUIRE((*selections)[0][0][0].get_allocator().resource() == memory);
}

//------------------------------------------------------------------------------
namespace {
template <typename Derived, typename T>
struct CRTPBase : public Blueprint<T> {
  DAG_TEMPLATE_HELPER()
  Derived *derived = static_cast<Derived *>(this);

  A &a() { return make_node<A>(); }
  B &b() dag_shared { return make_node<B>(derived->a()); }
  C &c() { return make_node<C>(derived->a(), derived->b()); }
  D &d() { return make_node<D>(derived->b(), derived->c()); }
};

template <typename Derived, typename T>
struct CRTPSystem : public CRTPBase<Derived, T> {
  DAG_TEMPLATE_HELPER()
  Derived *derived = static_cast<Derived *>(this);
  B &b() { return make_node<B>(derived->a()); }
};
template <typename T>
struct CRTPSystem2 : public CRTPSystem<CRTPSystem2<T>, T> {
  // D &config() { return d(); }
};

}  // namespace

TEST_CASE("factory can be overriden using curiously recurring template", "Blueprint") {
  // B
  auto factory = DagFactory<CRTPSystem2, B>();
  auto [entry, selections] = factory.create([](auto bp) -> auto & { return bp->d(); });

  REQUIRE(selections->size() == 2);
}

//------------------------------------------------------------------------------

namespace {
template <typename T>
struct System5 : public Blueprint<T> {
  DAG_TEMPLATE_HELPER()
  auto &a() dag_shared { return make_node<A>(); }
  B &b() { return make_node<B>(a()); }
  C &c() { return make_node<C>(a(), b()); }
  D &d() { return make_node<D>(b(), c()); }
  D &config() { return d(); }
};
}  // namespace

TEST_CASE("DAG_SHARED() can be used wth auto", "Blueprint") {
  auto factory = DagFactory<System5, A>();
  auto [entry, selections] = factory.create([](auto bp) -> auto & { return bp->d(); });

  REQUIRE(selections->size() == 1);
}

//------------------------------------------------------------------------------
namespace {
template <typename A, typename B>
struct Pair {
  explicit Pair(A &a, B &b) : m_a(a), m_b(b) {}
  A &m_a;
  B &m_b;
};
template <typename T>
struct System6 : public Blueprint<T> {
  DAG_TEMPLATE_HELPER()
  int &a() { return make_node<int>(100); }
  std::string &b() dag_shared { return make_node<std::string>("a"); }
  auto &c() { return make_node_t<Pair>(a(), b()); }
  auto &d() { return make_node_t<Pair>(b(), c()); }
};

}  // namespace

TEST_CASE("make_node_t() constructs template", "Blueprint") {
  auto factory = DagFactory<System6, int>();
  auto [entry, selections] = factory.create([](auto bp) -> auto & { return bp->d(); });

  REQUIRE(selections->size() == 1);
}
