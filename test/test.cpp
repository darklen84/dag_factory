#include <catch2/catch_test_macros.hpp>
#include <map>

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
  DAG_TEMPLATE_HELPER()
  A &a() { return make_node<A>(); }
  virtual B &b() DAG_SHARED(B) { return make_node<B>(a()); }
  C &c() { return make_node<C>(a(), b()); }
  D &d() { return make_node<D>(b(), c()); }
  D &config() { return d(); }
};

}  // namespace
//------------------------------------------------------------------------------
TEST_CASE(
    "factory functions tagged with DAG_SHARED() returns the same instance across multiple calls",
    "Blueprint") {
  auto [entry, selections] = DagFactory<System<B>>().create(std::mem_fn(&System<B>::config));
  ;

  REQUIRE(selections->size() == 1);
}

//------------------------------------------------------------------------------
TEST_CASE("normal factory function returns a new instance across multiple calls", "Blueprint") {
  auto [entry, selections] = DagFactory<System<A>>().create(std::mem_fn(&System<A>::config));

  REQUIRE(selections->size() == 2);
}
//------------------------------------------------------------------------------
TEST_CASE("factory can be overriden using runtime polymorphism", "Blueprint") {
  struct System2 : public System<B> {
    B &b() override { return make_node<B>(a()); }
  };

  auto [entry, selections] = DagFactory<System2>().create(std::mem_fn(&System2::config));

  REQUIRE(selections->size() == 2);
}

TEST_CASE("TypeToCollect in Blueprint is optional", "Blueprint") {
  struct System : public Blueprint<> {
    A &a() { return make_node<A>(); }
    B &b() { return make_node<B>(a()); }
    B &config() { return b(); }
  };

  auto [entry, selections] = DagFactory<System>().create(std::mem_fn(&System::config));
  REQUIRE(selections->size() == 0);
}

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

TEST_CASE("Blueprints with custom constructor are supported", "Blueprint") {
  struct System : public Blueprint<std::map<int, int>> {
    explicit System(int k, int v) : key(k), value(v) {}
    int key;
    int value;
    std::map<int, int> &a() DAG_SHARED(std::map<int, int>) {
      auto &map = make_node<std::map<int, int>>();
      map[key] = value;
      return map;
    }
    std::map<int, int> &config() { return a(); }
  };
  auto [entry, selections] = DagFactory<System>().create(std::mem_fn(&System::config), 1, 2);

  REQUIRE(selections->size() == 1);
  REQUIRE((*selections)[0]->operator[](1) == 2);
}

TEST_CASE("factory use and propagate the memory resource", "Resource") {
  struct System : public Blueprint<std::pmr::vector<std::pmr::string>> {
    TypeToSelect &a() {
      TypeToSelect &v = make_node<TypeToSelect>();
      v.push_back("a");
      return v;
    }
    TypeToSelect &config() { return a(); }
  };

  auto memory = std::pmr::new_delete_resource();
  auto [entry, selections] = DagFactory<System>(memory).create(std::mem_fn(&System::config));

  REQUIRE(selections->size() == 1);

  REQUIRE((*selections)[0]->get_allocator().resource() == memory);
  REQUIRE((*selections)[0][0].get_allocator().resource() == memory);
  REQUIRE((*selections)[0][0][0].get_allocator().resource() == memory);
}

//------------------------------------------------------------------------------
namespace {
template <typename Derived>
struct CRTPBase : public Blueprint<B> {
  DAG_TEMPLATE_HELPER()
  Derived *derived = static_cast<Derived *>(this);

  A &a() { return make_node<A>(); }
  B &b() DAG_SHARED(B) { return make_node<B>(derived->a()); }
  C &c() { return make_node<C>(derived->a(), derived->b()); }
  D &d() { return make_node<D>(derived->b(), derived->c()); }
};

template <typename Derived>
struct CRTPSystem : public CRTPBase<Derived> {
  DAG_TEMPLATE_HELPER()
  Derived *derived = static_cast<Derived *>(this);
  B &b() { return make_node<B>(derived->a()); }
};

struct CRTPSystem2 : public CRTPSystem<CRTPSystem2> {
  D &config() { return d(); }
};
}  // namespace

TEST_CASE("factory can be overriden using curiously recurring template", "Blueprint") {
  auto [entry, selections] = DagFactory<CRTPSystem2>().create(std::mem_fn(&CRTPSystem2::config));

  REQUIRE(selections->size() == 2);
}

//------------------------------------------------------------------------------
