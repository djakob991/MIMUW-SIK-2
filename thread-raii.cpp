
#include "thread-raii.hpp"

ThreadRAII::~ThreadRAII() {
  if (t.joinable()) {
    if (action == DtorAction::join) {
        t.join();
    } else {
        t.detach();
    }
  }
}

std::thread& ThreadRAII::get() {
  return t;
}