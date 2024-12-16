#include <iostream>
#include <ostream>

#include "dag/dag_factory.h"

namespace Oop {
/// [car_sim_oop]
struct Engine {
  virtual ~Engine() = default;
};

struct V6Engine : public Engine {
  ~V6Engine() override { std::cout << "V6 Engine destroyed" << std::endl; }
};

struct I4Engine : public Engine {
  ~I4Engine() override { std::cout << "I4 Engine destroyed" << std::endl; }
};

struct Transmission {
  virtual ~Transmission() = default;
};

struct AutoTransmission : public Transmission {
  explicit AutoTransmission(Engine &engine) {}
  ~AutoTransmission() override { std::cout << "Auto Transmission destroyed" << std::endl; }
};

struct CVTTransmission : public Transmission {
  explicit CVTTransmission(Engine &engine) {}
  ~CVTTransmission() override { std::cout << "CVT Transmission destroyed" << std::endl; }
};

struct CarSimulator {
  ~CarSimulator() { std::cout << "CarSimulator destroyed" << std::endl; }
  CarSimulator(Engine &engine, Transmission &transmission)
      : m_engine(engine), m_transmission(transmission) {}
  void start() { std::cout << "CarSimulator started" << std::endl; }
  Engine &m_engine;
  Transmission &m_transmission;
};
/// [car_sim_oop]

/// [car_sim_oop_blueprint]
template <typename T>
struct CarSimulatorBlueprint : public dag::Blueprint<T> {
  DAG_TEMPLATE_HELPER();
  CarSimulator &carSimulator() { return make_node<CarSimulator>(engine(), transmission()); }

  virtual Engine &engine() { return i4Engine(); }
  I4Engine &i4Engine() dag_shared { return make_node<I4Engine>(); }

  virtual Transmission &transmission() { return cvtTransmission(); }
  CVTTransmission &cvtTransmission() dag_shared { return make_node<CVTTransmission>(engine()); }
};
/// [car_sim_oop_blueprint]

/// [car_sim_oop_powerful_blueprint]
template <typename T>
struct PowerfulCarSimulatorBlueprint : public CarSimulatorBlueprint<T> {
  DAG_TEMPLATE_HELPER();
  Engine &engine() override { return this->v6Engine(); }
  V6Engine &v6Engine() dag_shared { return make_node<V6Engine>(); }

  Transmission &transmission() override { return this->autoTransmission(); }
  AutoTransmission &autoTransmission() dag_shared { return make_node<AutoTransmission>(engine()); }
};
/// [car_sim_oop_powerful_blueprint]

/// [car_sim_intercepter]
struct CarSimIntercepter : public dag::DefaultIntercepter {
  using dag::DefaultIntercepter::after_create;
  dag::unique_ptr<CVTTransmission> after_create(std::pmr::memory_resource *memory,
                                                dag::unique_ptr<CVTTransmission> tank) {
    std::cout << "[Intercepter] Intercepted CVTTransmission creation. " << std::endl;
    return std::move(tank);
  }
};
/// [car_sim_intercepter]
///[car_sim_creater]
struct CarSimCreator {
  template <typename T, typename... Args>
  dag::unique_ptr<T> create(std::pmr::memory_resource *memory, Args &&...args) {
    return Helper<T>::create(memory, std::forward<Args>(args)...);
  }

 private:
  template <typename T>
  struct Helper {
    template <typename... Args>
    static dag::unique_ptr<T> create(std::pmr::memory_resource *memory, Args &&...args) {
      return dag::make_unique_on_memory<T>(memory, std::forward<Args>(args)...);
    }
  };
};
template <>
struct CarSimCreator::Helper<I4Engine> {
  template <typename... Args>
  static dag::unique_ptr<I4Engine> create(std::pmr::memory_resource *memory, Args &&...args) {
    std::cout << "[Creater] Before create I4Engine. " << std::endl;
    return dag::make_unique_on_memory<I4Engine>(memory, std::forward<Args>(args)...);
  }
};
/// [car_sim_creater]

}  // namespace Oop

namespace Template {

struct V6Engine {
  ~V6Engine() { std::cout << "V6 Engine destroyed" << std::endl; }
  void start() { std::cout << "V6 Engine started" << std::endl; }
  unsigned int getSpeed() { return m_speed; }
  void setSpeed(unsigned int speed) {
    std::cout << "V6 Engine speed set to " << speed << std::endl;
    m_speed = speed;
  }
  unsigned int m_speed = 180;
};

struct I4Engine {
  ~I4Engine() { std::cout << "I4 Engine destroyed" << std::endl; }
  void start() { std::cout << "I4 Engine started" << std::endl; }
  unsigned int getSpeed() { return m_speed; }
  void setSpeed(unsigned int speed) {
    std::cout << "I4 Engine speed set to " << speed << std::endl;
    m_speed = speed;
  }
  unsigned int m_speed = 120;
};

template <typename Engine>
struct AutoTransmission {
  explicit AutoTransmission(Engine &engine) : m_engine(engine) {
    std::cout << "Auto Transmission created" << std::endl;
  }
  ~AutoTransmission() { std::cout << "Auto Transmission destroyed" << std::endl; }
  void shift(unsigned int gear) {
    std::cout << "Auto Transmission shifted to " << gear << std::endl;
    m_engine.setSpeed(gear * 1000);
  }
  Engine &m_engine;
};

template <typename Engine>
struct CVTTransmission {
  explicit CVTTransmission(Engine &engine) : m_engine(engine) {
    std::cout << "CVT Transmission created" << std::endl;
  }
  ~CVTTransmission() { std::cout << "CVT Transmission destroyed" << std::endl; }
  void shift(unsigned int gear) {
    std::cout << "CVT Transmission shifted to " << gear << std::endl;
    m_engine.setSpeed(gear * 800);
  }
  Engine &m_engine;
};

template <typename Engine, typename Transmission>
struct CarSimulator {
  CarSimulator(Engine &engine, Transmission &transmission)
      : m_engine(engine), m_transmission(transmission) {}
  void start() {
    m_engine.start();
    m_transmission.shift(1);
  }
  Engine &m_engine;
  Transmission &m_transmission;
};

template <typename Derived, typename T>
struct CarSimulatorBlueprint : public dag::Blueprint<T> {
  Derived *self = static_cast<Derived *>(this);
  DAG_TEMPLATE_HELPER();

  auto &carSimulator() { return make_node_t<CarSimulator>(self->engine(), self->transmission()); }
  auto &engine() dag_shared { make_node<I4Engine>(); }
  auto &transmission() dag_shared { return make_node_t<CVTTransmission>(self->engine()); }
};

template <typename T>
struct PowerfulCarSimulatorBlueprint
    : public CarSimulatorBlueprint<PowerfulCarSimulatorBlueprint<T>, T> {
  DAG_TEMPLATE_HELPER();
  auto &engine() dag_shared { return make_node<V6Engine>(); }
  auto &transmission() dag_shared { return make_node_t<AutoTransmission>(engine()); }
};

}  // namespace Template
/// [car_sim_oop_blueprint_test]
void run_sim_oop() {
  using namespace Oop;
  dag::DagFactory<CarSimulatorBlueprint> factory;
  std::cout << "===========Running OOP simulation==========" << std::endl;
  dag::unique_ptr<CarSimulator> simulator =
      factory.create([](auto bp) -> auto & { return bp->carSimulator(); });
  simulator->start();
  std::cout << "===========Ending OOP simulation==========" << std::endl;
}
/// [car_sim_oop_blueprint_test]

void run_powerful_sim_oop() {
  using namespace Oop;
  dag::DagFactory<PowerfulCarSimulatorBlueprint> factory;
  std::cout << "===========Running OOP simulation==========" << std::endl;
  dag::unique_ptr<CarSimulator> simulator =
      factory.create([](auto bp) -> auto & { return bp->carSimulator(); });
  simulator->start();
  std::cout << "===========Ending OOP simulation==========" << std::endl;
}

void run_sim_template() {
  using namespace Template;
  std::cout << "===========Running template simulation===========" << std::endl;
  dag::DagFactory<PowerfulCarSimulatorBlueprint> factory;
  auto simulator = factory.create([](auto bp) -> auto & { return bp->carSimulator(); });
  simulator->start();
  std::cout << "===========Ending template===========" << std::endl;
}
/// [car_sim_oop_blueprint_extension_test]
void run_sim_extensions() {
  using namespace Oop;
  CarSimCreator creater;
  CarSimIntercepter intercepter;
  dag::DagFactory<CarSimulatorBlueprint, dag::Select<dag::Nothing>, CarSimIntercepter,
                  CarSimCreator>
      factory(std::pmr::get_default_resource(), intercepter, creater);
  std::cout << "===========Running OOP simulation==========" << std::endl;
  dag::unique_ptr<CarSimulator> simulator =
      factory.create([](auto bp) -> auto & { return bp->carSimulator(); });
  simulator->start();
  std::cout << "===========Ending OOP simulation==========" << std::endl;
}
/// [car_sim_oop_blueprint_extension_test]
