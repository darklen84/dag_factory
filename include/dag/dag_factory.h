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
#define _DAG_SHARED(v, line)                                                                      \
  {                                                                                               \
    return this->template do_shared<v>([this]() -> v & { return DAG_COMBINE(factory, line)(); }); \
  }                                                                                               \
  v &DAG_COMBINE(factory, line)()

#define DAG_SHARED(v) _DAG_SHARED(v, __LINE__)

#define DAG_TEMPLATE_FACTORY()                                                 \
  template <typename NodeType, typename... Args>                               \
  NodeType &make_node(Args &&...args) {                                        \
    return this->template do_make_node<NodeType>(std::forward<Args>(args)...); \
  }

template <typename EntryPoint>
struct MutableDag : public Dag<EntryPoint> {
  explicit MutableDag(std::pmr::memory_resource *memory)
      : m_Components(memory), m_entryPoints(memory) {}
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
  friend class DagFactory<EntryPoint>;
};

template <typename EntryPoint>
struct DagFactory {
  template <typename T, typename... Args>
  T &make_node(Args &&...args) {
    T *o = new T(std::forward<Args>(args)...);
    m_Dag.m_Components.emplace_back(o, std::type_index(typeid(T)),
                                    [](void *p) { delete static_cast<T *>(p); });
    saveEntrypoint(o);
    return *o;
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

template <typename Derived>
struct Blueprint {
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
  DAG_TEMPLATE_FACTORY()
};

template <typename T>
struct BootStrapper {
  explicit BootStrapper(std::pmr::memory_resource *memory,
                        std::pmr::memory_resource *temporary_memory) {
    m_memory = memory;
    m_temporary_memory = temporary_memory;
  }
  BootStrapper(const BootStrapper<T> &) = default;

  unique_ptr<Dag<typename T::EntryPoint>> operator()(std::function<void(T *)> config) {
    std::pmr::polymorphic_allocator<MutableDag<typename T::EntryPoint>> alloc{m_memory};
    MutableDag<typename T::EntryPoint> *raw_dag = alloc.allocate(1);
    alloc.construct(raw_dag, m_memory);

    unique_ptr<MutableDag<typename T::EntryPoint>> dag{
        raw_dag, [alloc](void *p) mutable {
          auto obj = static_cast<MutableDag<typename T::EntryPoint> *>(p);
          alloc.destroy(obj);
          alloc.deallocate(obj, 1);
        }};

    std::pmr::unordered_map<std::type_index, void *> shared(m_temporary_memory);
    DagFactory<typename T::EntryPoint> factory{*dag, shared};
    T bluepoint;
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

/*
template <typename T>
unique_ptr<Dag<typename T::EntryPoint>> bootstrap(std::function<void(T *)> config) {
  unique_ptr<MutableDag<typename T::EntryPoint>> dag{
      new MutableDag<typename T::EntryPoint>(),
      [](void *p) { delete static_cast<MutableDag<typename T::EntryPoint> *>(p); }};
  std::unordered_map<std::type_index, void *> shared;
  DagFactory<typename T::EntryPoint> factory{*dag, shared};
  T bluepoint;
  bluepoint._hidden_state = &factory;
  config(&bluepoint);
  return dag;
}*/

}  // namespace dag