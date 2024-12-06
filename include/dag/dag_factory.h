#include <functional>
#include <memory>
#include <memory_resource>
#include <type_traits>
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

#define DAG_SHARED_IMP(...) _DAG_SHARED(__LINE__, __VA_ARGS__)

#define dag_shared DAG_SHARED_IMP(auto)
#define DAG_SHARED dag_shared

#define DAG_TEMPLATE_HELPER()                                                         \
  template <typename NodeType, typename... Args>                                      \
  NodeType &make_node(Args &&...args) {                                               \
    return this->template do_make_node<NodeType>(std::forward<Args>(args)...);        \
  }                                                                                   \
  template <template <typename...> typename NodeTemplate, typename... Args>           \
  auto &make_node_t(Args &&...args) {                                                 \
    return this->template do_make_node_t<NodeTemplate>(std::forward<Args>(args)...);  \
  }                                                                                   \
  template <template <typename...> typename BPTemplate, typename F, typename... Args> \
  auto &make_graph(F fn, Args &&...args) {                                            \
    return this->template do_make_graph<BPTemplate>(fn, std::forward<Args>(args)...); \
  }

template <typename TypeToSelect>
struct MutableDag : public Dag<TypeToSelect> {
  explicit MutableDag(std::pmr::memory_resource *memory)
      : m_Components(memory), m_entryPoints(memory) {}
  ~MutableDag() override {
    // components needs to be deleted in the reverse order of their creation.
    for (auto itr = m_Components.rbegin(); itr != m_Components.rend(); ++itr) {
      itr->reset(nullptr);
    }
  }
  MutableDag &operator=(MutableDag &&) = delete;
  const std::pmr::vector<TypeToSelect *> &selections() const override { return m_entryPoints; }

  // std::pmr::vector<std::tuple<void *, deleter>> m_Components;
  std::pmr::vector<unique_ptr<void>> m_Components;
  std::pmr::vector<TypeToSelect *> m_entryPoints;
};

template <typename T, typename... Args>
unique_ptr<T> make_unique_on_memory(std::pmr::memory_resource *memory, Args &&...args) {
  std::pmr::polymorphic_allocator<T> alloc{memory};
  T *raw = alloc.allocate(1);
  alloc.construct(raw, std::forward<Args>(args)...);
  return unique_ptr<T>(raw, [alloc](void *p) mutable {  // NOSONAR
    auto obj = static_cast<T *>(p);
    alloc.destroy(obj);
    alloc.deallocate(obj, 1);
  });
}

struct DefaultCreater {
  template <typename T, typename... Args>
  dag::unique_ptr<T> create(std::pmr::memory_resource *memory, Args &&...args) {
    return dag::make_unique_on_memory<T>(memory, std::forward<Args>(args)...);
  }
  static DefaultCreater &instance() {
    static DefaultCreater instance;
    return instance;
  }
};

struct DefaultIntercepter {
  template <typename... Args>
  void before_create(const std::tuple<Args &&...> &params) {
    // do nothing
  }
  template <typename T>
  dag::unique_ptr<T> after_create(std::pmr::memory_resource *memory, dag::unique_ptr<T> v) {
    return std::move(v);
  }

  static DefaultIntercepter &instance() {
    static DefaultIntercepter instance;
    return instance;
  }
};

template <typename T>
struct Select {
  using TypeToSelect = T;
};

template <typename Selection, typename Creater_t, typename Intercepter_t>
struct DagExtensions {
  using TypeToSelect = Selection;
  using Creater = Creater_t;
  using Intercepter = Intercepter_t;
};

template <typename Extentions>
struct DagContext {
  using TypeToSelect = typename Extentions::TypeToSelect;
  using Creater = typename Extentions::Creater;
  using Intercepter = typename Extentions::Intercepter;
  explicit DagContext(MutableDag<TypeToSelect> &dag, Creater &creater, Intercepter &intercepter)
      : m_Dag(dag), m_Creater(creater), m_Intercepter(intercepter) {}
  void saveEntrypoint(TypeToSelect *o) { m_Dag.m_entryPoints.push_back(o); }
  void saveEntrypoint(...) {}  // NOSONAR
  MutableDag<TypeToSelect> &m_Dag;
  Creater &m_Creater;
  Intercepter &m_Intercepter;
};
struct Nothing {};

struct FactoryUtils {};

template <typename Extensions>
struct Blueprint {
  void *_hidden_context = nullptr;
  using TypeToSelect = typename Extensions::TypeToSelect;
  template <typename NodeType, typename... Args>
  NodeType &do_make_node(Args &&...args) {
    auto context = static_cast<DagContext<Extensions> *>(_hidden_context);
    std::pmr::memory_resource *memory = context->m_Dag.m_entryPoints.get_allocator().resource();
    unique_ptr<NodeType> o =
        context->m_Creater.template create<NodeType>(memory, std::forward<Args>(args)...);
    o = context->m_Intercepter.template after_create<NodeType>(memory, std::move(o));
    NodeType *ptr = o.get();
    context->m_Dag.m_Components.emplace_back(std::move(o));
    context->saveEntrypoint(ptr);
    return *ptr;
  }

  template <template <typename...> typename NodeTemplate, typename... Args>
  auto &do_make_node_t(Args &&...args) {
    using NodeType = decltype(NodeTemplate(std::forward<Args &>(args)...));
    return do_make_node<NodeType>(std::forward<Args>(args)...);
  }

  template <template <typename> typename BP_Template, typename BP = BP_Template<Extensions>,
            typename F, typename RR = typename std::invoke_result<F, BP *>::type,
            typename R = typename std::remove_reference<RR>::type, typename... Args>
  R &do_make_graph(F initializer, Args &&...args) {
    static_assert(std::is_reference_v<RR>, "initializer must return a refernce of a dag node.");
    BP bluepoint{std::forward<Args>(args)...};
    bluepoint._hidden_context = _hidden_context;
    return initializer(&bluepoint);
  }

  DAG_TEMPLATE_HELPER()
};

template <template <typename> class BP_Template, typename Selecter = Select<Nothing>,
          typename Intercepter = DefaultIntercepter, typename Creater = DefaultCreater>
struct DagFactory {
  using Extensions = DagExtensions<typename Selecter::TypeToSelect, Creater, Intercepter>;
  using BP = BP_Template<Extensions>;
  explicit DagFactory(std::pmr::memory_resource *memory = std::pmr::get_default_resource(),
                      Intercepter &intercepter = DefaultIntercepter::instance(),
                      Creater &creater = DefaultCreater::instance())
      : m_memory(memory), m_intercepter(intercepter), m_creater(creater) {}
  DagFactory(const DagFactory<BP_Template, Selecter, Intercepter, Creater> &) = delete;

  template <typename F, typename RR = typename std::invoke_result<F, BP *>::type,
            typename R = typename std::remove_reference<RR>::type, typename... Args>
  auto create(F initializer, Args &&...args) {
    return doCreate<BP, F, RR, R>(m_memory, initializer, std::forward<Args>(args)...);
  }

 private:
  template <typename BP, typename F, typename RR, typename R, typename... Args>
  std::pair<unique_ptr<R>, const std::pmr::vector<typename BP::TypeToSelect *> *> createCommon(
      std::pmr::memory_resource *memory, F initializer, Args &&...args) {
    static_assert(std::is_reference_v<RR>, "initializer must return a refernce of a dag node.");
    std::pmr::polymorphic_allocator<MutableDag<typename BP::TypeToSelect>> alloc{memory};

    unique_ptr<MutableDag<typename BP::TypeToSelect>> dag =
        make_unique_on_memory<MutableDag<typename BP::TypeToSelect>>(memory, memory);
    DagContext<Extensions> factory{*dag, m_creater, m_intercepter};
    BP bluepoint{std::forward<Args>(args)...};
    bluepoint._hidden_context = &factory;
    R &result = initializer(&bluepoint);

    MutableDag<typename BP::TypeToSelect> *dag_address = dag.release();
    auto dag_deleter = dag.get_deleter();
    return {unique_ptr<R>(&result,
                          [dag_address, alloc](void *) mutable {  // NOSONAR
                            alloc.destroy(dag_address);
                            alloc.deallocate(dag_address, 1);
                          }),
            &dag_address->selections()};
  }

  template <typename BP, typename F, typename RR, typename R,
            typename = std::enable_if_t<!std::is_same_v<Nothing, typename BP::TypeToSelect>>,
            typename... Args>
  std::pair<unique_ptr<R>, const std::pmr::vector<typename BP::TypeToSelect *> *> doCreate(
      std::pmr::memory_resource *memory, F initializer, Args &&...args) {
    return createCommon<BP, F, RR, R>(memory, initializer, std::forward<Args>(args)...);
  }

  template <typename BP, typename F, typename RR, typename R,
            typename = std::enable_if_t<std::is_same_v<Nothing, typename BP::TypeToSelect>>,
            typename... Args>
  unique_ptr<R> doCreate(std::pmr::memory_resource *memory, F initializer, Args &&...args) {
    auto dag = createCommon<BP, F, RR, R>(memory, initializer, std::forward<Args>(args)...);
    return std::move(dag.first);
  }
  std::pmr::memory_resource *m_memory;
  Intercepter &m_intercepter;
  Creater &m_creater;
};
}  // namespace dag