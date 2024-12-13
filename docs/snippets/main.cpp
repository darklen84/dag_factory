#include <iostream>

extern void run_sim_oop();
extern void run_sim_template();
extern void run_sim_intercepter();
extern void run_sim_creater();

int main() {
  run_sim_oop();
  run_sim_template();
  run_sim_intercepter();
  run_sim_creater();
  return 0;
}
