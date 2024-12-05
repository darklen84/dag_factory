/// [classes]
struct B;
struct C;
struct A {
  explicit A(B& b1, B& b2) {}
};

struct B {
  explicit B(C& c) {}
};

struct C {};
/// [classes]