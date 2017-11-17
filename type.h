//
// Created by Shihao Jing on 10/28/17.
//

#ifndef PARALLEL_CONCURRENCY_TYPE_H
#define PARALLEL_CONCURRENCY_TYPE_H

#include <cstddef>
#include <memory>
#include <vector>

#define e_avail 0x01
#define e_lock  0x02
#define p_inner 0x03
#define p_term  0x04
#define p_lock  0x05
#define GARBAGE 0x06

#define DEPTH 2
#define EMAX  4
#define PINIT 4

#include "MurmurHash3.h"
#include <atomic>

struct item {
  int key;
  int value;
  item *next = nullptr;
};

template <typename T>
struct ListNode {
  std::atomic_int lock;
  ListNode* parent = nullptr;
  int idx = -1; // index in parent's buckets
  int size = 0; // capacity of buckets
  int hashkey;
  std::atomic<ListNode*> *buckets = nullptr;
  size_t count = 0; /* Number of Element Lists*/
  T items[EMAX];

  explicit ListNode(int hk) : hashkey(hk) {}

  explicit ListNode(int size_, int hk) : hashkey(hk), size(size_), buckets(new std::atomic<ListNode*>[size_]) {
    for (int i = 0; i < size; ++i) {
      buckets[i] = nullptr;
    }
  }

  uint32_t hash(T key) {
    uint32_t hv;
    MurmurHash3_x64_128(&key, sizeof(T), hashkey, &hv);
    return hv;
  }

  void release() {
    if (lock == e_lock)
      lock = e_avail;
    if (lock == p_lock)
      lock = p_term;
  }
};

#endif //PARALLEL_CONCURRENCY_TYPE_H
