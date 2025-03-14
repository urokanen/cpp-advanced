#include "signals.h"

#include <gtest/gtest.h>

#include <exception>
#include <optional>
#include <utility>

TEST(signal_test, trivial) {
  signals::signal<void()> sig;
  uint32_t got1 = 0;
  auto conn1 = sig.connect([&] { ++got1; });
  uint32_t got2 = 0;
  auto conn2 = sig.connect([&] { ++got2; });

  sig();

  EXPECT_EQ(got1, 1);
  EXPECT_EQ(got2, 1);

  sig();

  EXPECT_EQ(got1, 2);
  EXPECT_EQ(got2, 2);
}

TEST(signal_test, arguments) {
  signals::signal<void(int, int, int)> sig;
  auto conn = sig.connect([](int a, int b, int c) {
    EXPECT_EQ(a, 5);
    EXPECT_EQ(b, 6);
    EXPECT_EQ(c, 7);
  });

  int a = 5;
  sig(a, 6, 7);
}

TEST(signal_test, arguments_not_moved) {
  struct movable {
    explicit movable(int x)
        : x(x) {}

    movable(const movable& other)
        : x(other.x) {}

    movable(movable&& other)
        : x(std::exchange(other.x, -1)) {}

    movable& operator=(const movable&) = delete;
    movable& operator=(movable&&) = delete;

    int x;
  };

  signals::signal<void(movable)> sig;
  auto f = [](movable a) {
    EXPECT_EQ(a.x, 5);
  };

  auto conn1 = sig.connect(f);
  auto conn2 = sig.connect(f);

  sig(movable(5));
}

TEST(signal_test, empty_signal_move) {
  signals::signal<void()> a;
  signals::signal<void()> b = std::move(a);
  b = std::move(b);
}

TEST(signal_test, empty_connection_move) {
  signals::signal<void()>::connection a;
  signals::signal<void()>::connection b = std::move(a);
  b = std::move(b);
}

TEST(signal_test, disconnect) {
  signals::signal<void()> sig;
  uint32_t got1 = 0;
  auto conn1 = sig.connect([&] { ++got1; });
  uint32_t got2 = 0;
  auto conn2 = sig.connect([&] { ++got2; });

  sig();

  EXPECT_EQ(got1, 1);
  EXPECT_EQ(got2, 1);

  conn1.disconnect();
  sig();

  EXPECT_EQ(got1, 1);
  EXPECT_EQ(got2, 2);
}

TEST(signal_test, function_destroyed_after_disconnect) {
  bool destroyed = false;

  struct conn1_guard {
    explicit conn1_guard(bool& d)
        : d(d) {}

    ~conn1_guard() {
      d = true;
    }

    bool& d;
  };

  signals::signal<void()> sig;
  auto conn1 = sig.connect([&, g = conn1_guard(destroyed)] {});
  auto conn2 = sig.connect([&] {});

  conn1.disconnect();
  EXPECT_TRUE(destroyed);
}

TEST(signal_test, signal_move_ctor) {
  signals::signal<void()> sig_old;
  uint32_t got1 = 0;
  auto conn1 = sig_old.connect([&] { ++got1; });

  signals::signal<void()> sig_new = std::move(sig_old);

  sig_new();

  EXPECT_EQ(got1, 1);

  sig_old();

  EXPECT_EQ(got1, 1);
}

TEST(signal_test, signal_move_assign) {
  signals::signal<void()> sig1;
  uint32_t got1 = 0;
  auto conn1 = sig1.connect([&] { ++got1; });

  signals::signal<void()> sig2;
  uint32_t got2 = 0;
  auto conn2 = sig2.connect([&] { ++got2; });

  sig2 = std::move(sig1);

  sig2();

  EXPECT_EQ(got1, 1);
  EXPECT_EQ(got2, 0);

  sig1();

  EXPECT_EQ(got1, 1);
}

TEST(signal_test, signal_move_assign_self) {
  signals::signal<void()> sig1;
  uint32_t got1 = 0;
  auto conn1 = sig1.connect([&] { ++got1; });

  sig1 = std::move(sig1);

  sig1();

  EXPECT_EQ(got1, 1);
}

TEST(signal_test, connection_move_ctor) {
  signals::signal<void()> sig;
  uint32_t got1 = 0;
  auto conn1_old = sig.connect([&] { ++got1; });
  auto conn1_new = std::move(conn1_old);

  sig();

  EXPECT_EQ(got1, 1);
}

TEST(signal_test, connection_move_assign) {
  signals::signal<void()> sig;
  uint32_t got1 = 0;
  uint32_t got2 = 0;
  auto conn1 = sig.connect([&] { ++got1; });
  auto conn2 = sig.connect([&] { ++got2; });

  conn2 = std::move(conn1);

  sig();

  EXPECT_EQ(got1, 1);
  EXPECT_EQ(got2, 0);
}

TEST(signal_test, connection_move_assign_self) {
  signals::signal<void()> sig;
  uint32_t got1 = 0;
  auto conn1 = sig.connect([&] { ++got1; });

  conn1 = std::move(conn1);

  sig();

  EXPECT_EQ(got1, 1);
}

TEST(signal_test, connection_destructor) {
  signals::signal<void()> sig;
  uint32_t got1 = 0;
  auto conn1 = std::optional(sig.connect([&] { ++got1; }));
  uint32_t got2 = 0;
  auto conn2 = sig.connect([&] { ++got2; });

  sig();

  EXPECT_EQ(got1, 1);
  EXPECT_EQ(got2, 1);

  conn1.reset();
  sig();

  EXPECT_EQ(got1, 1);
  EXPECT_EQ(got2, 2);
}

TEST(signal_test, disconnect_inside_emit) {
  using connection = signals::signal<void()>::connection;
  signals::signal<void()> sig;
  uint32_t got1 = 0;
  auto conn1 = sig.connect([&] { ++got1; });
  uint32_t got2 = 0;
  connection conn2 = sig.connect([&] {
    ++got2;
    conn2.disconnect();
  });
  uint32_t got3 = 0;
  auto conn3 = sig.connect([&] { ++got3; });

  sig();

  EXPECT_EQ(got1, 1);
  EXPECT_EQ(got2, 1);
  EXPECT_EQ(got3, 1);

  sig();

  EXPECT_EQ(got1, 2);
  EXPECT_EQ(got2, 1);
  EXPECT_EQ(got3, 2);
}

TEST(signal_test, disconnect_other_connection_inside_emit) {
  using connection = signals::signal<void()>::connection;
  signals::signal<void()> sig;
  uint32_t got1 = 0;
  connection conn1 = sig.connect([&] { ++got1; });
  connection conn3;
  connection conn4;
  uint32_t got2 = 0;
  connection conn2 = sig.connect([&] {
    ++got2;
    conn1.disconnect();
    conn3.disconnect();
    conn4.disconnect();
  });
  uint32_t got3 = 0;
  conn3 = sig.connect([&] { ++got3; });
  uint32_t got4 = 0;
  conn4 = sig.connect([&] { ++got4; });

  sig();

  EXPECT_EQ(got2, 1);

  sig();

  EXPECT_GE(got1, 0);
  EXPECT_EQ(got2, 2);
  EXPECT_GE(got3, 0);
  EXPECT_GE(got4, 0);
}

TEST(signal_test, connection_destructor_inside_emit) {
  using connection = signals::signal<void()>::connection;
  signals::signal<void()> sig;
  uint32_t got1 = 0;
  auto conn1 = sig.connect([&] { ++got1; });
  uint32_t got2 = 0;
  std::optional<connection> conn2 = sig.connect([&] {
    ++got2;
    conn2.reset();
  });
  uint32_t got3 = 0;
  auto conn3 = sig.connect([&] { ++got3; });

  sig();

  EXPECT_EQ(got1, 1);
  EXPECT_EQ(got2, 1);
  EXPECT_EQ(got3, 1);

  sig();

  EXPECT_EQ(got1, 2);
  EXPECT_EQ(got2, 1);
  EXPECT_EQ(got3, 2);
}

TEST(signal_test, another_connection_destructor_inside_emit) {
  using connection = signals::signal<void()>::connection;
  signals::signal<void()> sig;
  uint32_t got1 = 0;
  auto conn1 = sig.connect([&] { ++got1; });
  uint32_t got2 = 0;
  std::optional<connection> conn3;
  auto conn2 = sig.connect([&] {
    ++got2;
    conn3.reset();
  });
  uint32_t got3 = 0;
  conn3 = std::optional(sig.connect([&] { ++got3; }));
  uint32_t got4 = 0;
  auto conn4 = sig.connect([&] { ++got4; });

  sig();

  EXPECT_EQ(got1, 1);
  EXPECT_EQ(got2, 1);
  EXPECT_EQ(got3, 0);
  EXPECT_EQ(got4, 1);

  sig();

  EXPECT_EQ(got1, 2);
  EXPECT_EQ(got2, 2);
  EXPECT_EQ(got3, 0);
  EXPECT_EQ(got4, 2);
}

TEST(signal_test, disconnect_before_emit) {
  signals::signal<void()> sig;
  uint32_t got1 = 0;
  auto conn1 = sig.connect([&] { ++got1; });

  conn1.disconnect();
  sig();
  EXPECT_EQ(got1, 0);
}

TEST(signal_test, destroy_signal_before_connection_01) {
  std::optional<signals::signal<void()>> sig(std::in_place);
  uint32_t got1 = 0;
  auto conn1 = sig->connect([&] { ++got1; });

  sig.reset();
}

TEST(signal_test, destroy_signal_before_connection_02) {
  std::optional<signals::signal<void()>> sig(std::in_place);
  uint32_t got1 = 0;
  auto conn1_old = sig->connect([&] { ++got1; });

  sig.reset();

  auto conn1_new = std::move(conn1_old);
}

TEST(signal_test, destroy_signal_inside_emit) {
  using connection = signals::signal<void()>::connection;

  std::optional<signals::signal<void()>> sig(std::in_place);
  uint32_t got1 = 0;
  connection conn1(sig->connect([&] { ++got1; }));
  uint32_t got2 = 0;
  connection conn2(sig->connect([&] {
    ++got2;
    sig.reset();
  }));
  uint32_t got3 = 0;
  connection conn3(sig->connect([&] { ++got3; }));

  (*sig)();

  EXPECT_EQ(got2, 1);
}

TEST(signal_test, recursive_emit) {
  std::optional<signals::signal<void()>> sig(std::in_place);
  uint32_t got1 = 0;
  auto conn1 = sig->connect([&] { ++got1; });
  uint32_t got2 = 0;
  auto conn2 = sig->connect([&] {
    ++got2;
    if (got2 == 1) {
      (*sig)();
    } else if (got2 == 2) {
      sig.reset();
    } else {
      FAIL() << "This branch should never execute";
    }
  });
  uint32_t got3 = 0;
  auto conn3 = sig->connect([&] { ++got3; });

  (*sig)();

  EXPECT_EQ(got2, 2);
}

TEST(signal_test, mutual_recursion) {
  using connection = signals::signal<void()>::connection;
  signals::signal<void()> sig;
  uint32_t got1 = 0;
  auto conn1 = sig.connect([&] { ++got1; });
  uint32_t got2 = 0;
  connection conn3;
  auto conn2 = sig.connect([&] {
    ++got2;
    if (got2 == 1) {
      sig();
    } else if (got2 == 2) {
      // do nothing
    } else if (got2 == 3) {
      conn3.disconnect();
    } else {
      FAIL() << "This branch should never execute";
    }
  });
  uint32_t got3 = 0;
  conn3 = sig.connect([&] {
    ++got3;
    if (got3 == 1 && got2 == 2) {
      sig();
    } else {
      FAIL() << "This branch should never execute";
    }
  });
  uint32_t got4 = 0;
  auto conn4 = sig.connect([&] { ++got4; });

  sig();

  EXPECT_EQ(got1, 3);
  EXPECT_EQ(got2, 3);
  EXPECT_EQ(got3, 1);
  EXPECT_EQ(got4, 3);
}

TEST(signal_test, exception_inside_emit) {
  struct test_exception : std::exception {};

  signals::signal<void()> sig;
  uint32_t got1 = 0;
  auto conn1 = sig.connect([&] { ++got1; });
  uint32_t got2 = 0;
  auto conn2 = sig.connect([&] {
    ++got2;
    if (got2 == 1) {
      throw test_exception();
    }
  });
  uint32_t got3 = 0;
  auto conn3 = sig.connect([&] { ++got3; });

  EXPECT_THROW(sig(), test_exception);
  EXPECT_EQ(got2, 1);

  got1 = 0;
  got3 = 0;

  sig();

  EXPECT_EQ(got1, 1);
  EXPECT_EQ(got2, 2);
  EXPECT_EQ(got3, 1);
}

TEST(signal_test, exception_inside_recursive_emit) {
  struct test_exception : std::exception {};

  signals::signal<void()> sig;
  uint32_t got1 = 0;
  auto conn1 = sig.connect([&] { ++got1; });
  uint32_t got2 = 0;
  auto conn2 = sig.connect([&] {
    ++got2;
    if (got2 == 1) {
      sig();
    } else if (got2 == 2) {
      throw test_exception();
    }
  });
  uint32_t got3 = 0;
  auto conn3 = sig.connect([&] { ++got3; });

  EXPECT_THROW(sig(), test_exception);
  EXPECT_EQ(got2, 2);

  got1 = 0;
  got3 = 0;

  sig();

  EXPECT_EQ(got1, 1);
  EXPECT_EQ(got2, 3);
  EXPECT_EQ(got3, 1);
}

TEST(signal_test, move_connection_inside_emit) {
  using connection = signals::signal<void()>::connection;

  signals::signal<void()> sig;
  uint32_t got1 = 0;
  connection conn1_new;

  std::optional<connection> conn1_old = sig.connect([&] {
    ++got1;
    if (got1 == 1) {
      conn1_new = std::move(*conn1_old);
      conn1_old.reset();
    }
  });

  sig();
  EXPECT_EQ(got1, 1);

  sig();
  EXPECT_EQ(got1, 2);
}

TEST(signal_test, move_other_connection_inside_emit) {
  using connection = signals::signal<void()>::connection;

  signals::signal<void()> sig;

  uint32_t got1 = 0;
  std::optional<connection> conn1_old;
  std::optional<connection> conn1_new;

  uint32_t got3 = 0;
  std::optional<connection> conn3_old;
  std::optional<connection> conn3_new;

  conn1_old = sig.connect([&] { ++got1; });

  uint32_t got2 = 0;
  connection conn2 = sig.connect([&] {
    ++got2;
    conn1_new = std::move(*conn1_old);
    conn3_new = std::move(*conn3_old);
    conn1_old.reset();
    conn3_old.reset();
  });

  conn3_old = sig.connect([&] { ++got3; });

  sig();
  EXPECT_EQ(got1, 1);
  EXPECT_EQ(got2, 1);
  EXPECT_EQ(got3, 1);
}

TEST(signal_test, move_connection_to_next_inside_emit) {
  using connection = signals::signal<void()>::connection;

  signals::signal<void()> sig;
  uint32_t got1 = 0;
  uint32_t got2 = 0;
  uint32_t got3 = 0;

  std::optional<connection> conn2;

  std::optional<connection> conn1 = sig.connect([&] {
    ++got1;
    if (got1 == 1) {
      conn2 = std::move(*conn1);
      conn1.reset();
    }
  });

  conn2 = sig.connect([&] { ++got2; });

  connection conn3 = sig.connect([&] { ++got3; });

  sig();
  EXPECT_EQ(got1, 1);
  EXPECT_EQ(got2, 0);
  EXPECT_EQ(got3, 1);

  sig();
  EXPECT_EQ(got1, 2);
  EXPECT_EQ(got2, 0);
  EXPECT_EQ(got3, 2);
}

TEST(signal_test, move_to_connection_inside_emit) {
  using connection = signals::signal<void()>::connection;

  signals::signal<void()> sig1;
  signals::signal<void()> sig2;
  uint32_t got1 = 0;
  uint32_t got2 = 0;
  uint32_t got3 = 0;

  std::optional<connection> conn1 = sig1.connect([&] { ++got1; });

  connection conn2 = sig2.connect([&] {
    ++got2;
    if (got2 == 1) {
      std::optional<connection>& conn1_ref = conn1;
      conn2 = std::move(*conn1);
      conn1_ref.reset();
    }
  });

  connection conn3 = sig2.connect([&] { ++got3; });

  sig2();
  EXPECT_EQ(got1, 0);
  EXPECT_EQ(got2, 1);
  EXPECT_EQ(got3, 1);

  sig2();
  EXPECT_EQ(got1, 0);
  EXPECT_EQ(got2, 1);
  EXPECT_EQ(got3, 2);

  sig1();
  EXPECT_EQ(got1, 1);
  EXPECT_EQ(got2, 1);
  EXPECT_EQ(got3, 2);
}

TEST(signal_test, move_to_connection_from_next_inside_emit) {
  using connection = signals::signal<void()>::connection;

  signals::signal<void()> sig;
  uint32_t got1 = 0;
  uint32_t got2 = 0;
  uint32_t got3 = 0;

  std::optional<connection> conn2;

  connection conn1 = sig.connect([&] {
    ++got1;
    if (got1 == 1) {
      std::optional<connection>& conn2_ref = conn2;
      conn1 = std::move(*conn2);
      conn2_ref.reset();
    }
  });

  conn2 = sig.connect([&] { ++got2; });

  connection conn3 = sig.connect([&] { ++got3; });

  sig();
  EXPECT_EQ(got1, 1);
  EXPECT_EQ(got2, 1);
  EXPECT_EQ(got3, 1);

  sig();
  EXPECT_EQ(got1, 1);
  EXPECT_EQ(got2, 2);
  EXPECT_EQ(got3, 2);
}

TEST(signal_test, move_signal_inside_emit) {
  using connection = signals::signal<void()>::connection;

  std::optional<signals::signal<void()>> sig_old(std::in_place);
  signals::signal<void()> sig_new;

  uint32_t got1 = 0;
  connection conn1 = sig_old->connect([&] {
    ++got1;
    if (got1 == 1) {
      sig_new = std::move(*sig_old);
      sig_old.reset();
    }
  });

  uint32_t got2 = 0;
  connection conn2 = sig_old->connect([&] { ++got2; });

  (*sig_old)();
  EXPECT_EQ(got1, 1);
  EXPECT_EQ(got2, 1);

  sig_new();
  EXPECT_EQ(got1, 2);
  EXPECT_EQ(got2, 2);
}
