#include <functional>
#include <memory>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>
#pragma one

namespace dag {
using deleter = void (*)(void *);
template <typename EntryPoint> struct DagFactory;

template <typename EntryPoint> struct Dag {
  virtual const std::vector<EntryPoint *> &entryPoints() const = 0;
  virtual ~Dag() = default;

protected:
  Dag() = default;
};

template <typename EntryPoint> struct MutableDag : public Dag<EntryPoint> {

  ~MutableDag() override {
    // components needs to be deleted in the reverse order of their creation.
    for (auto itr = m_Components.rbegin(); itr != m_Components.rend(); ++itr) {
      deleter fn = std::get<2>(*itr);
      fn(std::get<0>(*itr));
    }
  }

protected:
  const std::vector<EntryPoint *> &entryPoints() const override {
    return m_entryPoints;
  }

private:
  std::vector<std::tuple<void *, std::type_index, deleter>> m_Components;
  std::vector<EntryPoint *> m_entryPoints;
  friend class DagFactory<EntryPoint>;
};

template <typename EntryPoint> struct DagFactory {
  template <typename T, typename... Args> T &dedicated(Args &&...args) {
    T *o = new T(std::forward<Args>(args)...);
    m_Dag.m_Components.emplace_back(o, std::type_index(typeid(T)), [](void *p) {
      delete static_cast<T *>(p);
    });
    saveEntrypoint(o);
    return *o;
  }

  template <typename T, typename... Args> T &shared(Args &&...args) {
    void *&o = m_shared[std::type_index(typeid(T))];
    if (nullptr == o) {
      o = new T(std::forward<Args>(args)...);
      saveEntrypoint(static_cast<T *>(o));
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

template <typename T> struct Bluepoint {
  using EntryPoint = T;
  DagFactory<T> *m_factory = nullptr;

  template<typename R, typename ... Args> R& create(Args &&... args) {
    return m_factory->template dedicated<R>(std::forward<Args>(args)...);
  }

};

template <typename T>
std::unique_ptr<Dag<typename T::EntryPoint>>
bootstrap(std::function<void(T &)> config) {
  std::unique_ptr<MutableDag<typename T::EntryPoint>> dag =
      std::make_unique<MutableDag<typename T::EntryPoint>>();
  std::unordered_map<std::type_index, void *> shared;
  DagFactory<typename T::EntryPoint> factory{*dag, shared};
  T bluepoint;
  bluepoint.m_factory = &factory;
  config(bluepoint);
  return dag;
}

} // namespace dag