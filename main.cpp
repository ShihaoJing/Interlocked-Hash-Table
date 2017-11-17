//
// Created by Shihao Jing on 10/28/17.
//

#include "map.h"
#include "map_linked.h"
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <algorithm>
#include <random>
#include <unordered_map>

using namespace std;
int main() {
  map_linked map;
  int nthreads = 2;
  int ops = 10000000;
  int ratio = 60;

  auto do_work = [&]() {
    random_device rd;
    default_random_engine e(rd());
    uniform_int_distribution<int> re(0, INT32_MAX);
    for (int i = 0; i < ops; ++i) {
      printf("%d\n", i);
      int key = re(e);
      item *it = new item;
      it->key = key;
      if (re(e) % 100 < ratio)
        map.insert(it);
      else
        map.remove(it->key);
    }
  };

  std::vector<thread> threads;

  // timers for benchmark start and end
  std::chrono::high_resolution_clock::time_point start_time, end_time;

  start_time = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < nthreads; ++i) {
    threads.push_back(thread(do_work));
  }

  std::for_each(threads.begin(), threads.end(), [](thread &t) {
    t.join();
  });


  end_time = std::chrono::high_resolution_clock::now();

  // print the benchmark throughput
  auto diff = end_time - start_time;
  double exec_time = std::chrono::duration<double, std::milli> (diff).count();
  printf("latency: %f ms\n", exec_time);
  printf("Ops per seccond: %f \n", ops * nthreads / (exec_time / 1000));

}