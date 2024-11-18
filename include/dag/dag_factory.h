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

template <typename TypeToSelect>
struct DagContext;

template <typename TypeToSelect>
struct Dag {
  virtual const std::pmr::vector<TypeToSelect *> &selections() const = 0;
  virtual ~Dag() = default;

 protected:
  Dag() = default;
};

#define DAG_COMBINE(n, id) n##id
#define _DAG_SHARED(line, ...)                                          \
  {                                                                     \
    if (nullptr == DAG_COMBINE(singleton, line)) {                      \
      DAG_COMBINE(singleton, line) = &DAG_COMBINE(factory, line)();     \
    }                                                                   \
    using ResultRef = decltype(DAG_COMBINE(factory, line)());           \
    using ResultType = typename std::remove_reference<ResultRef>::type; \
    return *static_cast<ResultType *>(DAG_COMBINE(singleton, line));    \
  }                                                                     \
  void *DAG_COMBINE(singleton, line) = nullptr;                         \
  __VA_ARGS__ &DAG_COMBINE(factory, line)()

#define DAG_SHARED(...) _DAG_SHARED(__LINE__, __VA_ARGS__)

#define DAG_TEMPLATE_HELPER()                                                        \
  template <typename NodeType, typename... Args>                                     \
  NodeType &make_node(Args &&...args) {                                              \
    return this->template do_make_node<NodeType>(std::forward<Args>(args)...);       \
  }                                                                                  \
  template <template <typename...> typename NodeTemplate, typename... Args>          \
  auto &make_node_t(Args &&...args) {                                                \
    return this->template do_make_node_t<NodeTemplate>(std::forward<Args>(args)...); \
  }

template <typename TypeToSelect>
struct MutableDag : public Dag<TypeToSelect> {
  explicit MutableDag(std::pmr::memory_resource *memory)
      : m_Components(memory), m_entryPoints(memory) {}
  ~MutableDag() override {
    // components needs to be deleted in the reverse order of their creation.
    for (auto itr = m_Components.rbegin(); itr != m_Components.rend(); ++itr) {
      deleter fn = std::get<2>(*itr);
      fn(std::get<0>(*itr));
    }
  }
  const std::pmr::vector<TypeToSelect *> &selections() const override { return m_entryPoints; }

  std::pmr::vector<std::tuple<void *, std::type_index, deleter>> m_Components;
  std::pmr::vector<TypeToSelect *> m_entryPoints;
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
struct DagExtensions {
  using TypeToSelect = Selection;
};

template <typename Extentions>
struct DagContext {
  using TypeToSelect = typename Extentions::TypeToSelect;
  explicit DagContext(MutableDag<TypeToSelect> &dag) : m_Dag(dag) {}
  void saveEntrypoint(TypeToSelect *o) { m_Dag.m_entryPoints.push_back(o); }
  void saveEntrypoint(...) {}
  MutableDag<TypeToSelect> &m_Dag;
};
struct Nothing {};
template <typename Extensions>
struct Blueprint {
  void *_hidden_context = nullptr;
  using TypeToSelect = typename Extensions::TypeToSelect;
  template <typename NodeType, typename... Args>
  NodeType &do_make_node(Args &&...args) {
    auto context = static_cast<DagContext<Extensions> *>(_hidden_context);
    std::pmr::memory_resource *memory = context->m_Dag.m_entryPoints.get_allocator().resource();
    unique_ptr<NodeType> o = make_unique_on_memory<NodeType>(memory, std::forward<Args>(args)...);
    NodeType *ptr = o.release();
    context->m_Dag.m_Components.emplace_back(ptr, std::type_index(typeid(NodeType)),
                                             o.get_deleter());
    context->saveEntrypoint(ptr);
    return *ptr;
  }

  template <template <typename...> typename NodeTemplate, typename... Args>
  auto &do_make_node_t(Args &&...args) {
    using NodeType = decltype(NodeTemplate(std::forward<Args &>(args)...));
    return do_make_node<NodeType>(std::forward<Args>(args)...);
  }

  DAG_TEMPLATE_HELPER()
};
/*
template <typename BP>
struct DagFactory {
  explicit DagFactory(std::pmr::memory_resource *memory = std::pmr::get_default_resource()) {
    m_memory = memory;
  }
  DagFactory(const DagFactory<BP> &) = default;

  template <typename F, typename R = typename std::invoke_result<F, BP *>::type>
  auto test(F fn) -> R {
    static_assert(std::is_reference_v<R>, "initializer must return a refernce of a dag node.");
    return fn((BP *)nullptr);
  }

  template <typename F, typename RR = typename std::invoke_result<F, BP *>::type,
            typename R = typename std::remove_reference<RR>::type, typename... Args>
  std::pair<unique_ptr<R>, const std::pmr::vector<typename BP::TypeToSelect *> *> create(
      F initializer, Args &&...args) {
    static_assert(std::is_reference_v<RR>, "initializer must return a refernce of a dag node.");
    std::pmr::polymorphic_allocator<MutableDag<typename BP::TypeToSelect>> alloc{m_memory};

    unique_ptr<MutableDag<typename BP::TypeToSelect>> dag =
        make_unique_on_memory<MutableDag<typename BP::TypeToSelect>>(m_memory, m_memory);

    DagContext<BP> factory{*dag};
    BP bluepoint{std::forward<Args>(args)...};
    bluepoint._hidden_context = &factory;
    R &result = initializer(&bluepoint);

    MutableDag<typename BP::TypeToSelect> *dag_address = dag.release();
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
};
*/

template <template <typename> class BP_Template, typename TypeToSelect = Nothing>
struct DagFactory {
  using BP = BP_Template<DagExtensions<TypeToSelect>>;
  explicit DagFactory(std::pmr::memory_resource *memory = std::pmr::get_default_resource()) {
    m_memory = memory;
  }
  DagFactory(const DagFactory<BP_Template, TypeToSelect> &) = default;

  template <typename F, typename R = typename std::invoke_result<F, BP *>::type>
  auto test(F fn) -> R {
    static_assert(std::is_reference_v<R>, "initializer must return a refernce of a dag node.");
    return fn((BP *)nullptr);
  }

  template <typename F, typename RR = typename std::invoke_result<F, BP *>::type,
            typename R = typename std::remove_reference<RR>::type, typename... Args>
  std::pair<unique_ptr<R>, const std::pmr::vector<typename BP::TypeToSelect *> *> create(
      F initializer, Args &&...args) {
    static_assert(std::is_reference_v<RR>, "initializer must return a refernce of a dag node.");
    std::pmr::polymorphic_allocator<MutableDag<typename BP::TypeToSelect>> alloc{m_memory};

    unique_ptr<MutableDag<typename BP::TypeToSelect>> dag =
        make_unique_on_memory<MutableDag<typename BP::TypeToSelect>>(m_memory, m_memory);

    DagContext<BP> factory{*dag};
    BP bluepoint{std::forward<Args>(args)...};
    bluepoint._hidden_context = &factory;
    R &result = initializer(&bluepoint);

    MutableDag<typename BP::TypeToSelect> *dag_address = dag.release();
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
};

}  // namespace dag