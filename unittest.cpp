//
// Created by Shihao Jing on 10/29/17.
//

#include <iostream>
#include <string>
#include <thread>
#include "map.h"

using namespace std;

struct Test {
  Test();
  virtual void test() = 0;
};

vector<Test*> tests;
Test::Test() { tests.push_back(this); }

bool failed__ = false;
void error_print()
{
  cerr << endl;
}

template <typename A, typename ...Args>
void error_print(const A& a, Args...args)
{
  cerr << a;
  error_print(args...);
}

template <typename ...Args>
void fail(Args...args) { error_print(args...);failed__ = true; }

#define ASSERT_TRUE(x) if (!(x)) fail(__FILE__ ":", __LINE__, ": Assert fail: expected ", #x, " is true, at " __FILE__ ":",__LINE__)
#define ASSERT_EQUAL(a, b) if ((a) != (b)) fail(__FILE__ ":", __LINE__, ": Assert fail: expected ", (a), " actual ", (b),  ", " #a " == " #b ", at " __FILE__ ":",__LINE__)
#define ASSERT_NOTEQUAL(a, b) if ((a) == (b)) fail(__FILE__ ":", __LINE__, ": Assert fail: not expected ", (a), ", " #a " != " #b ", at " __FILE__ ":",__LINE__)
#define ASSERT_THROW(x) \
    try \
    { \
        x; \
        fail(__FILE__ ":", __LINE__, ": Assert fail: exception should be thrown"); \
    } \
    catch(std::exception&) \
    { \
    }



#define TEST(x) struct x:public Test{void test();}x##_; \
    void x::test()
#define DISABLE_TEST(x) struct test##x{void test();}x##_; \
    void test##x::test()

TEST(IntegralKey)
{
  {
    Map<int, int> map;
    map.Insert(1, 123);
    auto p = map.Lookup(1);
    ASSERT_EQUAL(*p.first, 1);
    ASSERT_EQUAL(*p.second, 123);
  }

  {
    Map<int, int> map;
    auto p = map.Lookup(1);
    ASSERT_EQUAL(p.first, NULL);
    ASSERT_EQUAL(p.second, NULL);
  }

  {
    Map<int, int> map;
    map.Insert(1, 123);
    map.Remove(1);
    auto p = map.Lookup(1);
    ASSERT_EQUAL(p.first, NULL);
    ASSERT_EQUAL(p.second, NULL);
  }

  {
    Map<int, int> m;
    for (int i = 0; i < 1000; ++i) {
      m.Insert(i, i);
    }

    for (int j = 0; j < 1000; ++j) {
      auto p = m.Lookup(j);
      ASSERT_EQUAL(*p.first, j);
      ASSERT_EQUAL(*p.second, j);
    }
  }
}



TEST(TwoThread)
{
  Map<int, int> m;
  auto do_work = [&m](int start, int end) {
    for (int i = start; i <= end; ++i) {
      m.Insert(i, i);
    }
    for (int i = start; i <= end; ++i) {
      auto p = m.Lookup(i);
      ASSERT_EQUAL(*p.first, i);
      ASSERT_EQUAL(*p.second, i);
    }
  };
  std::thread t1(do_work, 0, 500);
  std::thread t2(do_work, 501, 1000);
  t1.join();
  t2.join();
}

DISABLE_TEST(StringKey)
{
  Map<string, int> map;
  map.Insert("abc", 123);
  auto p = map.Lookup("abc");
  ASSERT_EQUAL(*p.first, "abc");
  ASSERT_EQUAL(*p.second, 123);
}

int main()
{
  bool failed = false;
  for (auto t: tests) {
    failed__ = false;
    try {
      t->test();
    }
    catch (exception &e) {
      fail(e.what());
    }
    if (failed__) {
      cerr << "Test Case";
      cerr << '\t' << typeid(*t).name() << endl;
      failed = true;
    }
  }
  if (!failed)
    cerr << "Congrats! All Test Cases passed.";
  return failed ? -1 : 0;
}