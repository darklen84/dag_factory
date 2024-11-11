#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>
#pragma one

namespace dag {
using deleter = void (*)(void *);
template <typename EntryPoint>
struct DagFactory;

template <typename EntryPoint>
struct Dag {
  virtual const std::vector<EntryPoint *> &entryPoints() const = 0;
  virtual ~Dag() = default;

 protected:
  Dag() = default;
};

#define DAG_COMBINE(n, id) n##id
#define _DAG_SHARED(v, line)                                                         \
  {                                                                                  \
    return dag->shared<v>([this]() -> v & { return DAG_COMBINE(factory, line)(); }); \
  }                                                                                  \
  v &DAG_COMBINE(factory, line)()

#define DAG_SHARED(v) _DAG_SHARED(v, __LINE__)

template <typename EntryPoint>
struct MutableDag : public Dag<EntryPoint> {
  ~MutableDag() override {
    // components needs to be deleted in the reverse order of their creation.
    for (auto itr = m_Components.rbegin(); itr != m_Components.rend(); ++itr) {
      deleter fn = std::get<2>(*itr);
      fn(std::get<0>(*itr));
    }
  }

 protected:
  const std::vector<EntryPoint *> &entryPoints() const override { return m_entryPoints; }

 private:
  std::vector<std::tuple<void *, std::type_index, deleter>> m_Components;
  std::vector<EntryPoint *> m_entryPoints;
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

  using shared_map = std::unordered_map<std::type_index, void *>;

  explicit DagFactory(MutableDag<EntryPoint> &dag, shared_map &shared)
      : m_Dag(dag), m_shared(shared) {}

 private:
  void saveEntrypoint(EntryPoint *o) { m_Dag.m_entryPoints.push_back(o); }
  void saveEntrypoint(...) {}

 private:
  MutableDag<EntryPoint> &m_Dag;
  shared_map &m_shared;
};

template <typename T>
struct Blueprint {
  using EntryPoint = T;
  DagFactory<T> *dag = nullptr;
};

template <typename T>
std::unique_ptr<Dag<typename T::EntryPoint>> bootstrap(std::function<void(T *)> config) {
  std::unique_ptr<MutableDag<typename T::EntryPoint>> dag =
      std::make_unique<MutableDag<typename T::EntryPoint>>();
  std::unordered_map<std::type_index, void *> shared;
  DagFactory<typename T::EntryPoint> factory{*dag, shared};
  T bluepoint;
  bluepoint.dag = &factory;
  config(&bluepoint);
  return dag;
}

}  // namespace dag