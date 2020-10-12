/* code adopted from Scott Meyers, Effective Modern C++ */

#include <thread>
#include <iostream>
#include <chrono>

class ThreadRAII {
public:
  enum class DtorAction { join, detach };
  ThreadRAII(std::thread&& t, DtorAction a): action(a), t(std::move(t)) {}
  ~ThreadRAII();
  std::thread& get();

private:
  DtorAction action;
  std::thread t;
};