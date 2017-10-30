//
// Created by Shihao Jing on 10/29/17.
//

#include <iostream>
#include <string>
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
  cerr<<a;
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
  Map<int, int> map;
  map.Insert(1, 123);
  auto p = map.Lookup(1);
  ASSERT_EQUAL(*p.first, 1);
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
  cerr << endl;
  return failed ? -1 : 0;
}