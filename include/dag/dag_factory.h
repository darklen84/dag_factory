#include <functional>
#include <memory>
#include <memory_resource>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>
#pragma one

namespace dag {
using deleter = std::function<void(void *)>;

template <typename T>
using unique_ptr = std::unique_ptr<T, deleter>;

template <typename EntryPoint>
struct DagFactory;

template <typename EntryPoint>
struct Dag {
  virtual const std::pmr::vector<EntryPoint *> &entryPoints() const = 0;
  virtual ~Dag() = default;

 protected:
  Dag() = default;
};

#define DAG_COMBINE(n, id) n##id
#define _DAG_SHARED(line, ...)                                               \
  {                                                                          \
    return this->template do_shared<__VA_ARGS__>(                            \
        [this]() -> __VA_ARGS__ & { return DAG_COMBINE(factory, line)(); }); \
  }                                                                          \
  __VA_ARGS__ &DAG_COMBINE(factory, line)()

#define DAG_SHARED(...) _DAG_SHARED(__LINE__, __VA_ARGS__)

#define DAG_TEMPLATE_HELPER()                                                  \
  template <typename NodeType, typename... Args>                               \
  NodeType &make_node(Args &&...args) {                                        \
    return this->template do_make_node<NodeType>(std::forward<Args>(args)...); \
  }

template <typename EntryPoint>
struct MutableDag : public Dag<EntryPoint> {
  explicit MutableDag(std::pmr::memory_resource *memory)
      : m_Components(memory), m_entryPoints(memory), m_memory(memory) {}
  ~MutableDag() override {
    // components needs to be deleted in the reverse order of their creation.
    for (auto itr = m_Components.rbegin(); itr != m_Components.rend(); ++itr) {
      deleter fn = std::get<2>(*itr);
      fn(std::get<0>(*itr));
    }
  }

 protected:
  const std::pmr::vector<EntryPoint *> &entryPoints() const override { return m_entryPoints; }

 private:
  std::pmr::vector<std::tuple<void *, std::type_index, deleter>> m_Components;
  std::pmr::vector<EntryPoint *> m_entryPoints;
  std::pmr::memory_resource *m_memory;
  friend class DagFactory<EntryPoint>;
};

template <typename T, typename... Args>
unique_ptr<T> make_unique_on_memory(std::pmr::memory_resource *memory, Args &&...args) {
  std::pmr::polymorphic_allocator<T> alloc{memory};
  T *raw = alloc.allocate(1);
  alloc.construct(raw, std::forward<Args>(args)...);
  return unique_ptr<T>(raw, [alloc](void *p) mutable {
    auto obj = static_cast<T *>(p);
    alloc.destroy(obj);
    alloc.deallocate(obj, 1);
  });
}

template <typename EntryPoint>
struct DagFactory {
  template <typename T, typename... Args>
  T &make_node(Args &&...args) {
    unique_ptr<T> o = make_unique_on_memory<T>(m_Dag.m_memory, std::forward<Args>(args)...);
    T *ptr = o.release();
    m_Dag.m_Components.emplace_back(ptr, std::type_index(typeid(T)), o.get_deleter());
    saveEntrypoint(ptr);
    return *ptr;
  }

  template <typename T>
  T &shared(std::function<T &()> fn) {
    void *&o = m_shared[std::type_index(typeid(T))];
    if (nullptr == o) {
      o = &fn();
    }
    return *static_cast<T *>(o);
  }

  using shared_map = std::pmr::unordered_map<std::type_index, void *>;

  explicit DagFactory(MutableDag<EntryPoint> &dag, shared_map &shared)
      : m_Dag(dag), m_shared(shared) {}

 private:
  void saveEntrypoint(EntryPoint *o) { m_Dag.m_entryPoints.push_back(o); }
  void saveEntrypoint(...) {}

 private:
  MutableDag<EntryPoint> &m_Dag;
  shared_map &m_shared;
};

struct EntrypointBase {
  virtual ~EntrypointBase() = default;
};

template <typename Derived>
struct Blueprint {
  using EntryPoint = EntrypointBase;
  void *_hidden_state = nullptr;

  template <typename NodeType, typename... Args>
  NodeType &do_make_node(Args &&...args) {
    auto dag = static_cast<DagFactory<typename Derived::EntryPoint> *>(_hidden_state);
    return dag->template make_node<NodeType>(std::forward<Args>(args)...);
  }
  template <typename T>
  T &do_shared(std::function<T &()> fn) {
    auto dag = static_cast<DagFactory<typename Derived::EntryPoint> *>(_hidden_state);
    return dag->shared(fn);
  }
  DAG_TEMPLATE_HELPER()
};

template <typename T>
struct BootStrapper {
  explicit BootStrapper(std::pmr::memory_resource *memory,
                        std::pmr::memory_resource *temporary_memory) {
    m_memory = memory;
    m_temporary_memory = temporary_memory;
  }
  BootStrapper(const BootStrapper<T> &) = default;
  template <typename... Args>
  unique_ptr<Dag<typename T::EntryPoint>> operator()(std::function<void(T *)> config,
                                                     Args &&...args) {
    std::pmr::polymorphic_allocator<MutableDag<typename T::EntryPoint>> alloc{m_memory};
    unique_ptr<MutableDag<typename T::EntryPoint>> dag =
        make_unique_on_memory<MutableDag<typename T::EntryPoint>>(m_memory, m_memory);

    std::pmr::unordered_map<std::type_index, void *> shared(m_temporary_memory);

    DagFactory<typename T::EntryPoint> factory{*dag, shared};
    T bluepoint{std::forward<Args>(args)...};
    bluepoint._hidden_state = &factory;
    config(&bluepoint);
    return dag;
  }

 private:
  std::pmr::memory_resource *m_memory;
  std::pmr::memory_resource *m_temporary_memory;
};

template <typename T>
BootStrapper<T> bootstrap(
    std::pmr::memory_resource *memory = std::pmr::get_default_resource(),
    std::pmr::memory_resource *temporary_memory = std::pmr::get_default_resource()) {
  return BootStrapper<T>(memory, temporary_memory);
}

}  // namespace dag