#include <catch2/catch.hpp>
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
struct System : public Blueprint<System<T>> {
  DAG_TEMPLATE_HELPER()
  using EntryPoint = T;
  A &a() { return make_node<A>(); }
  virtual B &b() DAG_SHARED(B) { return make_node<B>(a()); }
  C &c() { return make_node<C>(a(), b()); }
  D &d() { return make_node<D>(b(), c()); }
  void config() { d(); }
};

}  // namespace
//------------------------------------------------------------------------------
TEST_CASE(
    "factory functions tagged with DAG_SHARED() returns the same instance across multiple calls",
    "Blueprint") {
  auto dag = bootstrap<System<B>>()(std::mem_fn(&System<B>::config));
  ;

  REQUIRE(dag->entryPoints().size() == 1);
}

//------------------------------------------------------------------------------
TEST_CASE("normal factory function returns a new instance across multiple calls", "Blueprint") {
  auto dag = bootstrap<System<A>>()(std::mem_fn(&System<A>::config));

  REQUIRE(dag->entryPoints().size() == 2);
}
//------------------------------------------------------------------------------
TEST_CASE("factory can be overriden using runtime polymorphism", "Blueprint") {
  struct System2 : public System<B> {
    B &b() override { return make_node<B>(a()); }
  };

  auto dag = bootstrap<System2>()(std::mem_fn(&System2::config));

  REQUIRE(dag->entryPoints().size() == 2);
}

//------------------------------------------------------------------------------
namespace {
template <typename Derived>
struct CRTPBase : public Blueprint<Derived> {
  DAG_TEMPLATE_HELPER()
  using EntryPoint = B;
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
  void config() { d(); }
};
}  // namespace

TEST_CASE("factory can be overriden using curiously recurring template", "Blueprint") {
  auto dag = bootstrap<CRTPSystem2>()(std::mem_fn(&CRTPSystem2::config));

  REQUIRE(dag->entryPoints().size() == 2);
}

TEST_CASE("DAG_SHARE() accepts types with comma", "Blueprint") {
  struct System : public Blueprint<System> {
    using EntryPoint = std::map<int, int>;

    std::map<int, int> &a() DAG_SHARED(std::map<int, int>) {
      return make_node<std::map<int, int>>();
    }
    void config() { a(); }
  };
  auto dag = bootstrap<System>()(std::mem_fn(&System::config));

  REQUIRE(dag->entryPoints().size() == 1);
}

//------------------------------------------------------------------------------
TEST_CASE("factory use and propagate the memory resource", "Resource") {
  struct System : public Blueprint<System> {
    using EntryPoint = std::pmr::vector<std::pmr::string>;

    EntryPoint &a() {
      EntryPoint &v = make_node<EntryPoint>();
      v.push_back("a");
      return v;
    }
    void config() { a(); }
  };

  auto memory = std::pmr::new_delete_resource();
  auto dag = bootstrap<System>(memory)(std::mem_fn(&System::config));
  auto entries = dag->entryPoints();
  REQUIRE(entries.size() == 1);

  REQUIRE(entries[0]->get_allocator().resource() == memory);
  REQUIRE(dag->entryPoints()[0][0].get_allocator().resource() == memory);
  REQUIRE(dag->entryPoints()[0][0][0].get_allocator().resource() == memory);
}

/*
struct SystemA : public Blueprint<SystemA> {
  using EntryPoint = std::string;

  struct B : public Blueprint<B> {
    using EntryPoint = std::string;
    std::string &b() { return make_node<std::string>(); }
  };

  EntryPoint &a() { return this->B::b(); }
  void config() { b(); }
};*/