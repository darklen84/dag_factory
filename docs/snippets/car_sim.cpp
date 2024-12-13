#include <iostream>
#include <ostream>

#include "dag/dag_factory.h"

namespace Oop {
struct Tank {
  virtual ~Tank() = default;
  virtual void fill(unsigned int fuel) = 0;
};

struct GasolineTank : public Tank {
  GasolineTank() { std::cout << "Gasoline Tank created" << std::endl; }
  ~GasolineTank() override { std::cout << "Gasoline Tank destroyed" << std::endl; }
  void fill(unsigned int fuel) override {
    std::cout << "Gasoline Tank  filled: " << fuel << std::endl;
  }
};

struct DieselTank : public Tank {
  DieselTank() { std::cout << "Diesel Tank created" << std::endl; }
  ~DieselTank() override { std::cout << "Diesel Tank destroyed" << std::endl; }
  void fill(unsigned int fuel) override {
    std::cout << "Diesel Tank  filled： " << fuel << std::endl;
  }
};

struct Engine {
  virtual ~Engine() = default;
  virtual void start() = 0;
  virtual unsigned int getSpeed() = 0;
  virtual void setSpeed(unsigned int speed) = 0;
};

struct V6Engine : public Engine {
  explicit V6Engine(Tank &tank) : m_tank(tank) { std::cout << "V6 Engine created" << std::endl; }

  ~V6Engine() override { std::cout << "V6 Engine destroyed" << std::endl; }
  void start() override { std::cout << "V6 Engine started" << std::endl; }
  unsigned int getSpeed() override { return m_speed; }
  void setSpeed(unsigned int speed) override { m_speed = speed; }

  unsigned int m_speed = 180;
  Tank &m_tank;
};

struct I4Engine : public Engine {
  explicit I4Engine(Tank &tank) : m_tank(tank) { std::cout << "I4 Engine created" << std::endl; }
  ~I4Engine() override { std::cout << "I4 Engine destroyed" << std::endl; }
  void start() override { std::cout << "I4 Engine started" << std::endl; }
  unsigned int getSpeed() override { return m_speed; }
  void setSpeed(unsigned int speed) override {
    std::cout << "I4 Engine speed set to " << speed << std::endl;
    m_speed = speed;
  }

  unsigned int m_speed = 120;
  Tank &m_tank;
};

struct Transmission {
  virtual ~Transmission() = default;
  virtual void shift(unsigned int gear) = 0;
};

struct AutoTransmission : public Transmission {
  explicit AutoTransmission(Engine &engine) : m_engine(engine) {
    std::cout << "Auto Transmission created" << std::endl;
  }
  ~AutoTransmission() override { std::cout << "Auto Transmission destroyed" << std::endl; }
  void shift(unsigned int gear) override {
    std::cout << "Auto Transmission shifted to " << gear << std::endl;
    m_engine.setSpeed(gear * 1000);
  }
  Engine &m_engine;
};

struct CVTTransmission : public Transmission {
  explicit CVTTransmission(Engine &engine) : m_engine(engine) {
    std::cout << "CVT Transmission created" << std::endl;
  }
  ~CVTTransmission() override { std::cout << "CVT Transmission destroyed" << std::endl; }
  void shift(unsigned int gear) override {
    std::cout << "CVT Transmission shifted to " << gear << std::endl;
    m_engine.setSpeed(gear * 800);
  }
  Engine &m_engine;
};

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

template <typename T>
struct CarSimulatorBlueprint : public dag::Blueprint<T> {
  DAG_TEMPLATE_HELPER();
  CarSimulator &carSimulator() { return make_node<CarSimulator>(engine(), transmission()); }
  virtual Tank &tank() { return gasolineTank(); }
  GasolineTank &gasolineTank() dag_shared { return make_node<GasolineTank>(); }
  DieselTank &dieselTank() dag_shared { return make_node<DieselTank>(); }

  virtual Engine &engine() { return i4Engine(); }
  I4Engine &i4Engine() dag_shared { return make_node<I4Engine>(tank()); }
  V6Engine &v6Engine() dag_shared { return make_node<V6Engine>(tank()); }

  virtual Transmission &transmission() { return cvtTransmission(); }
  AutoTransmission &autoTransmission() dag_shared { return make_node<AutoTransmission>(engine()); }
  CVTTransmission &cvtTransmission() dag_shared { return make_node<CVTTransmission>(engine()); }
};

template <typename T>
struct PowerfulCarSimulatorBlueprint : public CarSimulatorBlueprint<T> {
  DAG_TEMPLATE_HELPER();
  Engine &engine() override { return this->v6Engine(); }
  Transmission &transmission() override { return this->autoTransmission(); }
};

struct CarSimIntercepter : public dag::DefaultIntercepter {
  using dag::DefaultIntercepter::after_create;
  dag::unique_ptr<GasolineTank> after_create(std::pmr::memory_resource *memory,
                                             dag::unique_ptr<GasolineTank> tank) {
    std::cout << "Intercepted GasolineTank creation. " << std::endl;
    tank->fill(100);
    return std::move(tank);
  }
};

struct CarSimCreator : public dag::DefaultCreater {
  using dag::DefaultCreater::create;
  template <typename T, typename... Args>
  dag::unique_ptr<T> create(std::pmr::memory_resource *memory, Args &&...args) {
    std::cout << "Intercepted Engine creation. " << std::endl;
    return dag::make_unique_on_memory<T>(memory, std::forward<Args>(args)...);
  }
};

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

void run_sim_oop() {
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

namespace {}
void run_sim_intercepter() {
  using namespace Oop;
  CarSimIntercepter intercepter;
  dag::DagFactory<PowerfulCarSimulatorBlueprint, dag::Select<dag::Nothing>, CarSimIntercepter>
      factory(std::pmr::get_default_resource(), intercepter);
  std::cout << "===========Running OOP simulation==========" << std::endl;
  dag::unique_ptr<CarSimulator> simulator =
      factory.create([](auto bp) -> auto & { return bp->carSimulator(); });
  simulator->start();
  std::cout << "===========Ending OOP simulation==========" << std::endl;
}