#include "bimap.h"
#include "test-classes.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <map>
#include <random>

namespace {

template <typename T>
std::vector<std::pair<T, T>> eliminate_same(std::vector<T>& lefts, std::vector<T>& rights, std::mt19937& e) {
  // std::sort(lefts.begin(), lefts.end());
  auto last = std::unique(lefts.begin(), lefts.end());
  lefts.erase(last, lefts.end());
  last = std::unique(rights.begin(), rights.end());
  rights.erase(last, rights.end());

  size_t min = std::min(lefts.size(), rights.size());
  lefts.resize(min);
  rights.resize(min);

  std::shuffle(lefts.begin(), lefts.end(), e);
  std::shuffle(rights.begin(), rights.end(), e);

  std::vector<std::pair<T, T>> res(min);
  for (size_t i = 0; i < min; i++) {
    res[i] = {lefts[i], rights[i]};
  }

  return res;
}

} // namespace

static constexpr uint32_t seed = 1488228;

TEST_CASE("[Randomized] - Comparison") {
  INFO("Seed used for randomized compare test is " << seed);

  bimap<uint32_t, uint32_t> b1;
  bimap<uint32_t, uint32_t> b2;

  size_t total = 40000;
  std::mt19937 e(seed);
  std::vector<uint32_t> lefts(total), rights(total);
  for (size_t i = 0; i < total; i++) {
    lefts[i] = e();
    rights[i] = e();
  }
  auto future_insertions = eliminate_same(lefts, rights, e);

  std::shuffle(future_insertions.begin(), future_insertions.end(), e);
  for (auto p : future_insertions) {
    b1.insert(p.first, p.second);
  }

  std::shuffle(future_insertions.begin(), future_insertions.end(), e);
  for (auto p : future_insertions) {
    b2.insert(p.first, p.second);
  }

  CHECK(b1.size() == b2.size());
  CHECK(b1 == b2);
}

TEST_CASE("[Randomized] - Check invariants") {
  INFO("Seed used for randomized invariant test is " << seed);
  bimap<int, int> b;

  std::mt19937 e(seed);
  size_t ins = 0, skip = 0, total = 50'000;
  for (size_t i = 0; i < total; i++) {
    auto op = e() % 10;
    if (op > 2) {
      ins++;
      b.insert(e(), e());
    } else {
      if (b.empty()) {
        skip++;
        continue;
      }
      auto it = b.end_left();
      while (it == b.end_left()) {
        it = b.lower_bound_left(e());
      }
      b.erase_left(it);
    }
    if (i % 100 == 0) {
      int previous = *b.begin_left();
      for (auto it = ++b.begin_left(); it != b.end_left(); it++) {
        CHECK(previous < *it);
        previous = *it;
      }
      previous = *b.begin_right();
      for (auto it = ++b.begin_right(); it != b.end_right(); it++) {
        CHECK(previous < *it);
        previous = *it;
      }
    }
  }
  INFO("Invariant check stats:");
  INFO("Performed " << ins << " insertions and " << total - ins - skip << " erasures. " << skip << " skipped.");
}

TEST_CASE("[Randomized] - Compare to 2 maps") {
  INFO("Seed used for randomized cmp2map test is " << seed);

  bimap<int, int> b;
  std::map<int, int> left_view, right_view;

  std::mt19937 e(seed);
  size_t ins = 0, skip = 0, total = 60'000;
  for (size_t i = 0; i < total; i++) {
    unsigned int op = e() % 10;
    if (op > 2) {
      ins++;
      // insertion
      int l = e(), r = e();
      b.insert(l, r);
      left_view.insert({l, r});
      right_view.insert({r, l});
    } else {
      // erasure
      if (b.empty()) {
        skip++;
        continue;
      }
      auto it = b.end_left();
      while (it == b.end_left()) {
        it = b.lower_bound_left(e());
      }
      CHECK(left_view.erase(*it) == 1);
      CHECK(right_view.erase(*it.flip()) == 1);
      b.erase_left(it);
    }
    if (i % 100 == 0) {
      // check
      CHECK(b.size() == left_view.size());
      CHECK(b.size() == right_view.size());
      auto lit = b.begin_left();
      auto mlit = left_view.begin();
      for (; lit != b.end_left() && mlit != left_view.end(); lit++, mlit++) {
        CHECK(*lit == mlit->first);
        CHECK(*lit.flip() == mlit->second);
      }
    }
  }
  INFO("Comparing to maps stat:");
  INFO("Performed " << ins << " insertions and " << total - ins - skip << " erasures. " << skip << " skipped.");
}
