#include "fault-injection.h"

#include <catch2/catch_test_macros.hpp>

#include <iostream>
#include <memory>
#include <vector>

namespace {

void* injected_allocate(size_t count) {
  if (should_inject_fault()) {
    throw std::bad_alloc();
  }

  void* ptr = std::malloc(count);
  if (!ptr) {
    throw std::bad_alloc();
  }

  return ptr;
}

void injected_deallocate(void* ptr) {
  std::free(ptr);
}

struct fault_injection_context {
  std::vector<size_t> skip_ranges;
  size_t error_index = 0;
  size_t skip_index = 0;
  bool fault_registered = false;
};

thread_local bool disabled = false;
thread_local fault_injection_context* context = nullptr;

void dump_state() {
#if 0
  fault_injection_disable dg;
  std::cout << "skip_ranges: {";
  if (!context->skip_ranges.empty()) {
    std::cout << context->skip_ranges[0];
    for (size_t i = 1; i != context->skip_ranges.size(); ++i) {
      std::cout << ", " << context->skip_ranges[i];
    }
  }
  std::cout << "}\nerror_index: " << context->error_index << "\nskip_index: " << context->skip_index << '\n'
            << std::flush;
#endif
}

void faulty_run_advance() {
  fault_injection_disable dg;

  dump_state();

  if (!context->fault_registered) {
    FAIL("Caught an unexpected injected fault");
  }

  context->skip_ranges.resize(context->error_index);
  ++context->skip_ranges.back();
  context->error_index = 0;
  context->skip_index = 0;
  context->fault_registered = false;
}

template <typename F>
struct scope_guard {
  explicit scope_guard(F on_scope_exit)
      : on_scope_exit(std::move(on_scope_exit)) {}

  ~scope_guard() {
    on_scope_exit();
  }

private:
  F on_scope_exit;
};

void faulty_run_raw(const std::function<void()>& f) {
  if (context) {
    fault_injection_disable dg;
    FAIL("Recursive faulty runs are not supported");
  }
  fault_injection_context ctx;
  context = &ctx;
  scope_guard ctx_nullifier([]() { context = nullptr; });

  f();
}

} // namespace

bool should_inject_fault() {
  if (!context) {
    return false;
  }

  if (disabled) {
    return false;
  }

  fault_injection_disable dg;

  REQUIRE(context->error_index <= context->skip_ranges.size());
  if (context->error_index == context->skip_ranges.size()) {
    ++context->error_index;
    context->skip_ranges.push_back(0);
    context->fault_registered = true;
    return true;
  }

  REQUIRE(context->skip_index <= context->skip_ranges[context->error_index]);
  if (context->skip_index == context->skip_ranges[context->error_index]) {
    ++context->error_index;
    context->skip_index = 0;
    context->fault_registered = true;
    return true;
  }

  ++context->skip_index;
  return false;
}

void fault_injection_point() {
  if (should_inject_fault()) {
    fault_injection_disable dg;
    throw injected_fault("injected fault");
  }
}

void faulty_run(const std::function<void()>& f) {
  faulty_run_raw([&f] {
    while (true) {
      try {
        f();
      } catch (const injected_fault&) {
        faulty_run_advance();
        continue;
      } catch (const std::bad_alloc&) {
        faulty_run_advance();
        continue;
      }
      if (context->fault_registered) {
        fault_injection_disable dg;
        FAIL("Could not catch an injected fault");
      }
      break;
    }
  });
}

void assert_nothrow(const std::function<void()>& f) {
  faulty_run_raw([&f] {
    try {
      f();
    } catch (...) {
      fault_injection_disable dg;
      FAIL("Exception thrown while no were expected");
    }
  });
}

fault_injection_disable::fault_injection_disable()
    : was_disabled(disabled) {
  disabled = true;
}

void fault_injection_disable::reset() {
  disabled = was_disabled;
}

fault_injection_disable::~fault_injection_disable() {
  reset();
}

void* operator new(size_t count) {
  return injected_allocate(count);
}

void* operator new(size_t count, const std::nothrow_t&) noexcept {
  return malloc(count);
}

void* operator new[](size_t count) {
  return injected_allocate(count);
}

void* operator new[](size_t count, const std::nothrow_t&) noexcept {
  return malloc(count);
}

void operator delete(void* ptr) noexcept {
  injected_deallocate(ptr);
}

void operator delete[](void* ptr) noexcept {
  injected_deallocate(ptr);
}

void operator delete(void* ptr, size_t) noexcept {
  injected_deallocate(ptr);
}

void operator delete[](void* ptr, size_t) noexcept {
  injected_deallocate(ptr);
}
