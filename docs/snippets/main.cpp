#include <iostream>

extern void run_sim_oop();
extern void run_powerful_sim_oop();
extern void run_sim_template();
extern void run_sim_extensions();

int main() {
  run_sim_oop();
  run_powerful_sim_oop();
  run_sim_template();
  run_sim_extensions();
  return 0;
}
