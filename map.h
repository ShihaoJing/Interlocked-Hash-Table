//
// Created by Shihao Jing on 10/28/17.
//

#ifndef PARALLEL_CONCURRENCY_MAP_H
#define PARALLEL_CONCURRENCY_MAP_H

#include "type.h"
#include <random>
#include <cassert>
#include <memory>
#include <string>

using namespace std;

random_device rd;
default_random_engine e(rd());
uniform_int_distribution<int> re(0, INT32_MAX);

template <typename T>
class Map {
  typedef ListNode<T>* nptr;
private:
  std::mutex count_lock;
  size_t count;
  nptr root;

  int backPath(nptr p) {
    if (p == root)
      return 0;
    return 1 + backPath(p->parent);
  }

  nptr EToP(nptr elist) {
    nptr p = new ListNode<T>(PINIT, re(e));
    p->lock = p_inner;
    p->idx = elist->idx;
    p->parent = elist->parent;

    for (int i = 0; i < elist->count; ++i) {
      T it = elist->items[i];
      uint32_t hv = p->hash(it);
      int idx = hv % p->size;
      // Elementlist
      nptr next = p->buckets[idx];
      if (next == nullptr) {
        //on nil bucket, insert new ElementList
        next = new ListNode<T>(re(e));
        next->lock = e_avail;
        next->idx = idx;
        next->parent = p;
        next->items[next->count++] = it;

        //install new Elementlist
        p->buckets[idx].store(next);
        //p->buckets[idx] = next;
      }
      else if (next->count < EMAX) {
        next->items[next->count++] = it;
      }
      else {
        throw std::logic_error("unexpected happend");
      }
    }
    return p;
  }

  nptr PToBiggerP(nptr cur) {
    int new_size = cur->size;
    nptr p = nullptr;
    bool done = false;
    while (!done) {
      new_size *= 2;
      p = new ListNode<T>(new_size, re(e));
      p->lock = p_term;
      p->idx = cur->idx;
      p->parent = cur->parent;

      int i = 0;
      for (; i < cur->size; ++i) {
        if (cur->buckets[i] != nullptr) {
          nptr elist = cur->buckets[i];
          bool complete = true;
          for (int j = 0; j < elist->count; ++j) {
            T it = elist->items[j];
            uint32_t hv = p->hash(it);
            int idx = hv % p->size;
            // Elementlist
            nptr next = p->buckets[idx];
            if (next == nullptr) {
              //on nil bucket, insert new ElementList
              next = new ListNode<T>(re(e));
              next->lock = e_avail;
              next->parent = p;
              next->idx = idx;
              next->items[next->count++] = it;

              //install new Elementlist
              p->buckets[idx].store(next);
              //p->buckets[idx] = next;
            }
            else if (next->count < EMAX) {
              next->items[next->count++] = it;
            }
            else {
              // ElementList is full, keep expanding parent's PointList's size
              complete = false;
              break;
            }
          }
          if (!complete)
            break;
        }
      }
      if (i == cur->size) {
        done = true;
      }
    }
    return p;
  }


  nptr getList(const T &key) {
    nptr l = nullptr;
    nptr cur = root;
    while (true) {
      uint32_t hv = cur->hash(key);
      int idx = hv % cur->size;
      nptr next = cur->buckets[idx];
      if (next == nullptr) {
        // Allocate a Element List
        nptr new_ele_list = new ListNode<T>(re(e));
        new_ele_list->lock = e_avail;
        new_ele_list->parent = cur;
        new_ele_list->idx = idx;

        if (l == nullptr) {
          new_ele_list->lock = e_lock;
          l = new_ele_list;
        }

        nptr null = nullptr;
        if (cur->buckets[idx].compare_exchange_strong(null, new_ele_list)) {
          return l;
        }
      }
      else if (next->lock == p_term) {
        int tst_val = p_term;
        int new_val = p_lock;
        if (next->lock.compare_exchange_strong(tst_val, new_val))
          l = cur = next;
      }
      else if (next->lock == p_inner) {
        cur = next;
      }
      else if (next->lock == e_avail) {
        int tst_val = e_avail;
        int new_val = e_lock;
        if (l != nullptr || next->lock.compare_exchange_strong(tst_val, new_val)) {
          if (l == nullptr)
            l = next;
          if (next->count < EMAX) {
            return l;
          }
          for (int i = 0; i < next->count; ++i) {
            if (next->items[i] == key) {
              return l;
            }
          }

          if (cur->lock == p_inner) {
            nptr p = EToP(next);

            //printf("backPath called\n");
            int depth = backPath(p);
            //printf("depth %u\n", depth);
            if (depth == DEPTH) {
              p->lock = p_term;
              l = p;
            }

            cur->buckets[idx].store(p);
            assert(cur->buckets[idx] == p);
          }
          else {
            // resize locked PointerList
            nptr p = PToBiggerP(cur);
            p->lock = p_lock;
            l = p;
            p->parent->buckets[p->idx].store(p);
            assert(p->parent->buckets[p->idx] == p);
            cur = p;
          }
        }
      }
    }
  }

  nptr getList_nolock(const T &key) {
    nptr cur = root;
    while (true) {
      uint32_t hv = cur->hash(key);
      nptr next = cur->buckets[hv % cur->size];
      if (next->lock == e_lock || next->lock == e_avail) {
        return next;
      }
      else {
        cur = next;
      }
    }
  }


public:
  Map() : root(new ListNode<T>(PINIT, re(e))), count(0) { root->lock = p_inner; }

  nptr acquire(int key) {
    return getList(key);
  }

  void release(nptr lock) {
    lock->release();
  }

  T find(T key) {
    T it;
    nptr lock = getList(key);
    nptr elist = getList_nolock(key);
    for (int i = 0; i < elist->count; ++i) {
      if (elist->items[i] == key)
      {
        it = elist->items[i];
        break;
      }
    }
    lock->release();
    return it;
  }

  /*void insert(item *it) {
    ListNode *elist = getList_nolock(it->key);
    elist->items[elist->count++] = it;
  }*/

  bool insert(T it) {
    nptr lock = getList(it);
    nptr elist = getList_nolock(it);
    for (int i = 0; i < elist->count; ++i) {
      if (elist->items[i] == it) /* item already exist */
      {
        lock->release();
        return false;
      }
    }
    count_lock.lock();
    ++count;
    count_lock.unlock();
    elist->items[elist->count++] = it;
    lock->release();
    return true;
  }

  bool erase(T key) {
    nptr lock = getList(key);
    nptr elist = getList_nolock(key);
    for (int i = 0; i < elist->count; ++i) {
      if (elist->items[i] == key)
      {
        count_lock.lock();
        --count;
        count_lock.unlock();
        elist->items[i] = elist->items[--elist->count];
        lock->release();
        return true;
      }
    }
    lock->release();
    return false;
  }

  size_t size() {
    return count;
  }

};
#endif //PARALLEL_CONCURRENCY_MAP_H
