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
  auto factory = DagFactory<System, Select<B>>();
  auto [entry, selections] = factory.create([](auto bp) -> auto & { return bp->d(); });

  REQUIRE(selections->size() == 1);
}

//------------------------------------------------------------------------------
TEST_CASE("normal factory function returns a new instance across multiple calls", "Blueprint") {
  auto factory = DagFactory<System, Select<A>>();
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
  auto factory = DagFactory<System, Select<A>>();
  auto [entry, selections] = factory.create([](auto bp) -> auto & { return bp->d(); });
  REQUIRE(selections->size() == 2);
}

TEST_CASE("TypeToCollect in Blueprint is optional", "Blueprint") {
  auto factory = DagFactory<System>();
  auto entry = factory.create([](auto bp) -> auto & { return bp->d(); });
  REQUIRE(entry != nullptr);
}

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
  auto factory = DagFactory<System3, Select<std::map<int, int>>>();
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
  auto factory = DagFactory<System4, Select<std::pmr::vector<std::pmr::string>>>(memory);
  auto [entry, selections] = factory.create([](auto bp) -> auto & { return bp->a(); });

  REQUIRE(selections->size() == 1);

  REQUIRE((*selections)[0]->get_allocator().resource() == memory);
  REQUIRE((*selections)[0][0].get_allocator().resource() == memory);
  REQUIRE((*selections)[0][0][0].get_allocator().resource() == memory);
}

namespace {
template <typename T>
struct SubGraph7 : public Blueprint<T> {
  DAG_TEMPLATE_HELPER()
  explicit SubGraph7(A &a) : m_a(a) {}
  A &m_a;

  A &a() { return m_a; }
  B &b() dag_shared { return make_node<B>(a()); }
};

template <typename T>
struct System7 : public Blueprint<T> {
  DAG_TEMPLATE_HELPER();
  C &c() { return make_node<C>(this->a(), this->b()); }
  D &d() { return make_node<D>(this->b(), this->c()); }
  A &a() { return make_node<A>(); }

  B &b() {
    return this->template do_make_graph<SubGraph7>([](auto bp) -> auto & { return bp->b(); }, a());
  }
};

}  // namespace
TEST_CASE("Blueprints can create sub-graphs", "Blueprint") {
  auto factory = DagFactory<System7, Select<A>>();
  auto [entry, selections] = factory.create([](auto bp) -> auto & { return bp->d(); });

  REQUIRE(selections->size() == 3);  // one a created in the main graph and two in the subgraph
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
struct CRTPSystem2 : public CRTPSystem<CRTPSystem2<T>, T> {};

}  // namespace

TEST_CASE("factory can be overriden using curiously recurring template", "Blueprint") {
  // B
  auto factory = DagFactory<CRTPSystem2, Select<B>>();
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
  auto factory = DagFactory<System5, Select<A>>();
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
  auto factory = DagFactory<System6, Select<int>>();
  auto [entry, selections] = factory.create([](auto bp) -> auto & { return bp->d(); });

  REQUIRE(selections->size() == 1);
}

//------------------------------------------------------------------------------
namespace {

template <typename T>
struct System8 : public Blueprint<T> {
  DAG_TEMPLATE_HELPER()
  A &a() { return make_node<A>(); }
  B &b() dag_shared { return make_node<B>(a()); }
  C &c() { return make_node<C>(a(), b()); }
  D &d() { return make_node<D>(b(), c()); }
  D &config() { return d(); }
};
struct MyIntercepter : public DefaultIntercepter {
  int called = 0;
  template <typename T>
  dag::unique_ptr<T> after_create(std::pmr::memory_resource *memory, dag::unique_ptr<T> v) {
    ++called;
    return std::move(v);
  }
};
struct MyCreater : public DefaultCreater {
  int called = 0;
  template <typename T, typename... Args>
  dag::unique_ptr<T> create(std::pmr::memory_resource *memory, Args &&...args) {
    ++called;
    return dag::make_unique_on_memory<T>(memory, std::forward<Args>(args)...);
  }
};
}  // namespace
TEST_CASE("intercepter is called for every node created", "Blueprint") {
  MyIntercepter intercepter;
  auto factory =
      DagFactory<System8, Select<A>, MyIntercepter>(std::pmr::get_default_resource(), intercepter);
  auto [entry, selections] = factory.create([](auto bp) -> auto & { return bp->d(); });
  REQUIRE(intercepter.called == 5);
  REQUIRE(selections->size() == 2);
}

TEST_CASE("creater is called for every node", "Blueprint") {
  MyCreater creater;
  auto factory = DagFactory<System8, Select<A>, DefaultIntercepter, MyCreater>(
      std::pmr::get_default_resource(), DefaultIntercepter::instance(), creater);
  auto [entry, selections] = factory.create([](auto bp) -> auto & { return bp->d(); });
  REQUIRE(creater.called == 5);
  REQUIRE(selections->size() == 2);
}