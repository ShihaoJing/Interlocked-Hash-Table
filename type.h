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

template <typename Key, typename T>
struct ListNode {
  using key_type = Key;
  using mapped_type = T;
  // common fields
  std::atomic_int lock;
  std::shared_ptr<ListNode> parent;
  size_t idx; // index in parent's buckets
  // fields of PointList
  size_t size;
  uint32_t hashkey;
  std::vector<std::shared_ptr<ListNode>> buckets;
  // fields of ElementLists
  size_t count;
  key_type keys[EMAX];
  mapped_type values[EMAX];

  ListNode() : lock(p_inner), parent(nullptr), size(PINIT), buckets(size), count(0), hashkey(rand()) { }

  ListNode(int l, size_t s, std::shared_ptr<ListNode> par, size_t i = -1)
      : lock(l), size(s), parent(par), idx(i), buckets(size), count(0), hashkey(rand()) { }

  uint32_t hash(key_type key) {
    uint32_t out;
    MurmurHash3_x86_32(&key, sizeof(key), hashkey, &out);
    return out;
  }

  void release() {
    if (lock == e_lock)
      lock = e_avail;
    if (lock == p_lock)
      lock = p_term;
  }
};

#endif //PARALLEL_CONCURRENCY_TYPE_H
