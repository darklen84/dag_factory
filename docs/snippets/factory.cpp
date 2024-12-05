#include <memory>
namespace {
struct B;
struct C;
struct A {
  explicit A(std::unique_ptr<B> b1, std::unique_ptr<B> b2) {}
};

struct B {
  explicit B(std::shared_ptr<C> c) {}
};

struct C {};

/// [factory]
struct Factory {
  std::unique_ptr<A> a() { return std::make_unique<A>(b(), b()); }
  std::unique_ptr<B> b() { return std::make_unique<B>(c()); }

  std::shared_ptr<C> c_ptr;
  std::shared_ptr<C> c() {
    if (nullptr == c_ptr) {
      c_ptr = std::make_shared<C>();
    }
    return c_ptr;
  }
};

static void test() {
  Factory factory;
  std::unique_ptr<A> obj = factory.a();
}
/// [factory]

}  // namespace