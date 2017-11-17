//
// Created by Shihao Jing on 11/16/17.
//

#ifndef PARALLEL_CONCURRENCY_MAP_LINKED_H
#define PARALLEL_CONCURRENCY_MAP_LINKED_H

#include "type.h"
#include <atomic>
#include <mutex>

#define HASHSIZE(n) ((size_t)1<<(n))
#define HASHMASK(n) (n-1)

#define DEFAULT_LOCK_CAP 16
#define DEFAULT_CAP 16

using namespace std;

template <typename T>
class map_linked {
  size_t capacity;
  size_t lock_capacity;
  mutex *locks;
  item **buckets;
  uint32_t hashseed = rand();
  mutex count_lock;
  size_t count;

  uint32_t hash(int key) {
    uint32_t hv;
    MurmurHash3_x64_128(&key, sizeof(int), hashseed, &hv);
    return hv;
  }

  void acquire(int key) {
    uint32_t hv = hash(key);
    locks[hv & HASHMASK(lock_capacity)].lock();
  }

  void release(int key) {
    uint32_t hv = hash(key);
    locks[hv & HASHMASK(lock_capacity)].unlock();
  }

  void insert_nolock(item *it) {
    uint32_t hv = hash(it->key);
    size_t idx = hv & HASHMASK(capacity);

    item *sentinel = buckets[idx];
    item *cur = sentinel;

    while (cur->next) {
      if (cur->next->key == it->key)
        throw logic_error("unexpected happened");
      cur = cur->next;
    }

    cur->next = it;
  }

  bool policy() {
    return count / capacity > 4;
  }

  void resize() {

    for (int i = 0; i < lock_capacity; ++i) {
      locks[i].lock();
    }

    size_t old_cap = capacity;
    item **old_buckets = buckets;

    capacity = (capacity << 1);
    buckets = new item*[capacity];
    for (int i = 0; i < capacity; ++i) {
      buckets[i] = new item; // sentinel node
    }

    for (int i = 0; i < old_cap; ++i) {
      item *sentinel = old_buckets[i];
      item *cur = sentinel->next;
      while (cur) {
        item *next = cur->next;
        cur->next = nullptr;
        insert_nolock(cur);
        cur = next;
      }
    }

    for (int i = 0; i < lock_capacity; ++i) {
      locks[i].unlock();
    }

  }

public:

  map_linked(int hashpower = DEFAULT_CAP,
             int lock_hashpower = DEFAULT_LOCK_CAP) : capacity(HASHSIZE(hashpower)), lock_capacity(HASHSIZE(lock_hashpower)),
                                                      buckets(new item*[HASHSIZE(hashpower)]),
                                                      locks(new mutex[HASHSIZE(lock_hashpower)]),
                                                      count(0)
  {
    for (int i = 0; i < capacity; ++i) {
      buckets[i] = new item; // sentinel node
    }
  }

  bool insert(item *it) {
    acquire(it->key);

    uint32_t hv = hash(it->key);
    int idx = hv & HASHMASK(capacity);

    item *sentinel = buckets[idx];
    item *cur = sentinel;

    while (cur->next) {
      if (cur->next->key == it->key) {
        release(it->key);
        return false;
      }
      cur = cur->next;
    }

    cur->next = it;
    count_lock.lock();
    ++count;
    count_lock.unlock();
    release(it->key);

    if (policy())
      resize();

    return true;
  }

  bool erase(int key) {
    acquire(key);

    uint32_t hv = hash(key);
    int idx = hv & HASHMASK(capacity);

    item *sentinel = buckets[idx];
    item *cur = sentinel;

    while (cur->next) {
      if (cur->next->key == key) {
        cur->next = cur->next->next;
        count_lock.lock();
        --count;
        count_lock.unlock();
        release(key);
        return true;
      }
      cur = cur->next;
    }

    release(key);
    return false;
  }

  size_t size() {
    return count;
  }

};

#endif //PARALLEL_CONCURRENCY_MAP_LINKED_H
