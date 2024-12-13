#include <iostream>

extern void run_sim_oop();
extern void run_sim_template();
extern void run_sim_intercepter();

int main() {
  run_sim_oop();
  run_sim_template();
  run_sim_intercepter();
  return 0;
}
