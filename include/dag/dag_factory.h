#include <functional>
#include <memory>
#include <memory_resource>
#include <type_traits>
#include <typeindex>
#include <utility>
#include <vector>
#pragma one

namespace dag {
using deleter = std::function<void(void *)>;

template <typename T>
using unique_ptr = std::unique_ptr<T, deleter>;

template <typename Selection>
struct DagContext;

template <typename Selection>
struct Dag {
  virtual const std::pmr::vector<Selection *> &selections() const = 0;
  virtual ~Dag() = default;

 protected:
  Dag() = default;
};

#define DAG_COMBINE(n, id) n##id
#define _DAG_SHARED(line, ...)                                      \
  {                                                                 \
    if (nullptr == DAG_COMBINE(singleton, line)) {                  \
      DAG_COMBINE(singleton, line) = &DAG_COMBINE(factory, line)(); \
    }                                                               \
    return *DAG_COMBINE(singleton, line);                           \
  }                                                                 \
  __VA_ARGS__ *DAG_COMBINE(singleton, line) = nullptr;              \
  __VA_ARGS__ &DAG_COMBINE(factory, line)()

#define DAG_SHARED(...) _DAG_SHARED(__LINE__, __VA_ARGS__)

#define DAG_TEMPLATE_HELPER()                                                  \
  template <typename NodeType, typename... Args>                               \
  NodeType &make_node(Args &&...args) {                                        \
    return this->template do_make_node<NodeType>(std::forward<Args>(args)...); \
  }

template <typename Selection>
struct MutableDag : public Dag<Selection> {
  explicit MutableDag(std::pmr::memory_resource *memory)
      : m_Components(memory), m_entryPoints(memory), m_memory(memory) {}
  ~MutableDag() override {
    // components needs to be deleted in the reverse order of their creation.
    for (auto itr = m_Components.rbegin(); itr != m_Components.rend(); ++itr) {
      deleter fn = std::get<2>(*itr);
      fn(std::get<0>(*itr));
    }
  }
  const std::pmr::vector<Selection *> &selections() const override { return m_entryPoints; }

 private:
  std::pmr::vector<std::tuple<void *, std::type_index, deleter>> m_Components;
  std::pmr::vector<Selection *> m_entryPoints;
  std::pmr::memory_resource *m_memory;
  friend class DagContext<Selection>;
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

template <typename Selection>
struct DagContext {
  template <typename T, typename... Args>
  T &make_node(Args &&...args) {
    unique_ptr<T> o = make_unique_on_memory<T>(m_Dag.m_memory, std::forward<Args>(args)...);
    T *ptr = o.release();
    m_Dag.m_Components.emplace_back(ptr, std::type_index(typeid(T)), o.get_deleter());
    saveEntrypoint(ptr);
    return *ptr;
  }

  explicit DagContext(MutableDag<Selection> &dag) : m_Dag(dag) {}

 private:
  void saveEntrypoint(Selection *o) { m_Dag.m_entryPoints.push_back(o); }
  void saveEntrypoint(...) {}

 private:
  MutableDag<Selection> &m_Dag;
};
struct SelectionBase {
  virtual ~SelectionBase() = default;
};

template <typename Derived>
struct Blueprint {
  void *_hidden_state = nullptr;
  using TypeToSelect = SelectionBase;
  template <typename NodeType, typename... Args>
  NodeType &do_make_node(Args &&...args) {
    auto dag = static_cast<DagContext<typename Derived::TypeToSelect> *>(_hidden_state);
    return dag->template make_node<NodeType>(std::forward<Args>(args)...);
  }
  DAG_TEMPLATE_HELPER()
};

template <typename T>
struct DagFactory {
  explicit DagFactory(
      std::pmr::memory_resource *memory = std::pmr::get_default_resource(),
      std::pmr::memory_resource *temporary_memory = std::pmr::get_default_resource()) {
    m_memory = memory;
    m_temporary_memory = temporary_memory;
  }
  DagFactory(const DagFactory<T> &) = default;

  template <typename F, typename R = typename std::invoke_result<F, T *>::type>
  auto test(F fn) -> R {
    static_assert(std::is_reference_v<R>, "initializer must return a refernce of a dag node.");
    return fn((T *)nullptr);
  }

  template <typename F, typename RR = typename std::invoke_result<F, T *>::type,
            typename R = typename std::remove_reference<RR>::type, typename... Args>
  std::pair<unique_ptr<R>, const std::pmr::vector<typename T::TypeToSelect *> *> create(
      F initializer, Args &&...args) {
    static_assert(std::is_reference_v<RR>, "initializer must return a refernce of a dag node.");
    std::pmr::polymorphic_allocator<MutableDag<typename T::TypeToSelect>> alloc{m_memory};

    unique_ptr<MutableDag<typename T::TypeToSelect>> dag =
        make_unique_on_memory<MutableDag<typename T::TypeToSelect>>(m_memory, m_memory);

    DagContext<typename T::TypeToSelect> factory{*dag};
    T bluepoint{std::forward<Args>(args)...};
    bluepoint._hidden_state = &factory;
    R &result = initializer(&bluepoint);

    MutableDag<typename T::TypeToSelect> *dag_address = dag.release();
    auto dag_deleter = dag.get_deleter();
    return {unique_ptr<R>(&result,
                          [dag_address, alloc](void *) mutable {
                            alloc.destroy(dag_address);
                            alloc.deallocate(dag_address, 1);
                          }),
            &dag_address->selections()};
  }

 private:
  std::pmr::memory_resource *m_memory;
  std::pmr::memory_resource *m_temporary_memory;
};

}  // namespace dag