//
// Created by Shihao Jing on 10/28/17.
//

#ifndef PARALLEL_CONCURRENCY_MAP_H
#define PARALLEL_CONCURRENCY_MAP_H

#include <memory>
#include "type.h"

template <typename Key, typename T>
class Map {
  using listptr = std::shared_ptr<ListNode<Key, T>>;
  using key_type = Key;
  using mapped_type = T;
private:
  listptr root;

  // Return a locker ElementList where key may exist
  std::pair<listptr, listptr>
  GetElist(listptr map, key_type key);


  // Insert a (key, val) to map, or update value if key already exists
  void Insert(listptr map, key_type key, mapped_type value);

  std::pair<std::shared_ptr<key_type>, std::shared_ptr<mapped_type>>
  Lookup(listptr map, key_type key);

  void Remove(listptr map, key_type key);

  int backPath(listptr p, listptr map) {
    if (p == map)
      return 0;
    return 1 + backPath(p->parent, map);
  }

  listptr EToP(listptr next, int l);

  listptr PToBiggerP(listptr cur);

public:
  Map() : root(new ListNode<Key,T>) { }
  // Insert a (key, val) to map, or update value if key already exists
  void Insert(key_type key, mapped_type value);

  std::pair<std::shared_ptr<key_type>, std::shared_ptr<mapped_type>>
  Lookup(key_type key);

  void Remove(key_type key);
};

template <typename Key, typename T>
std::pair<typename Map<Key,T>::listptr, typename Map<Key,T>::listptr>
Map<Key, T>::GetElist(listptr map, key_type key)
{
  listptr found, l;
  listptr cur = map;
  while (true) {
    uint32_t hash_value = cur->hash(key);
    size_t idx = cur->hash(key) % cur->size;
    listptr next = cur->buckets[idx];
    if (next == nullptr) {
      found = std::make_shared<ListNode<Key, T>>(e_avail, PINIT, cur, idx);
      if (l == nullptr) {
        found->lock = e_lock;
        l = found;
      }
      listptr null;
      if (std::atomic_compare_exchange_weak(&cur->buckets[idx], &null, found))
        return {found, l};
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
          return {next, l};
        }
        for (int i = 0; i < next->count; ++i) {
          if (next->keys[i] == key) {
            return {next, l};
          }
        }
        next->lock = GARBAGE;
        //no locked pointerlist
        if (cur->lock == p_inner) {
          auto p = EToP(next, p_inner);
          if (backPath(p, map) == DEPTH) {
            p->lock = p_term;
            l = p;
          }
          p->idx = idx;
          std::atomic_store(&cur->buckets[idx], p);
        }
        // resize locked PointerList
        else {
          auto p = PToBiggerP(cur);
          cur->lock = GARBAGE;
          std::atomic_store(&(cur->parent->buckets[cur->idx]), p);
          cur = p;
        }
      }
    }
  }
};

template <typename Key, typename T>
std::pair<std::shared_ptr<Key>, std::shared_ptr<T>>
Map<Key, T>::Lookup(listptr map, key_type key) {
  std::shared_ptr<key_type> ret_key;
  std::shared_ptr<mapped_type> ret_val;
  auto node = GetElist(map, key);
  listptr elist = node.first;
  listptr lock = node.second;
  for (int i = 0; i < elist->count; ++i) {
    if (elist->keys[i] == key) {
      ret_key = std::make_shared<key_type>(elist->keys[i]);
      ret_val = std::make_shared<mapped_type>(elist->values[i]);
    }
  }
  lock->release();
  return {ret_key, ret_val};
}

template <typename Key, typename T>
std::pair<std::shared_ptr<Key>, std::shared_ptr<T>>
Map<Key, T>::Lookup(key_type key) {
  return Lookup(root, key);
}

template <typename Key, typename T>
void Map<Key, T>::Insert(listptr map, key_type key, mapped_type value) {
  auto node = GetElist(map, key);
  listptr elist = node.first;
  listptr lock = node.second;
  for (int i = 0; i < elist->count; ++i) {
    if (elist->keys[i] == key) {
      elist->values[i] = value;
      lock->release();
    }
  }
  elist->keys[elist->count] = key;
  elist->values[elist->count] = value;
  ++elist->count;
  lock->release();
}

template <typename Key, typename T>
void Map<Key, T>::Insert(key_type key, mapped_type value) {
  return Insert(root, key, value);
}

template <typename Key, typename T>
void Map<Key, T>::Remove(listptr map, key_type key) {
  auto node = GetElist(map, key);
  listptr elist = node.first;
  listptr lock = node.second;
  for (int i = 0; i < elist->count; ++i) {
    if (elist->keys[i] == key) {
      elist->keys[i] = elist->keys[elist->count-1];
      elist->values[i] = elist->values[elist->count-1];
      --elist->count;
      break;
    }
  }
  lock->release();
}

template <typename Key, typename T>
void Map<Key, T>::Remove(key_type key) {
  Remove(root, key);
}


template <typename Key, typename T>
std::shared_ptr<ListNode<Key, T>> Map<Key, T>::EToP(listptr next, int l) {
  listptr p = std::make_shared<ListNode<Key, T>>(l, PINIT, next->parent);
  for (int i = 0; i < next->count; ++i) {
    Insert(p, next->keys[i], next->values[i]);
  }
  return p;
}

template <typename Key, typename T>
std::shared_ptr<ListNode<Key, T>> Map<Key, T>::PToBiggerP(listptr cur) {
  size_t new_size = cur->size;
  listptr p;
  //rehash old to new
  bool done = false;
  while (!done) {
    new_size *= 2;
    p = std::make_shared<ListNode<Key, T>>(p_term, new_size, cur->parent, cur->idx);
    int i = 0;
    for (; i < cur->size; ++i) {
      if (cur->buckets[i]) {
        auto elist = cur->buckets[i];
        bool complete = true;
        for (int j = 0; j < elist->count; ++j) {
          key_type &key = elist->keys[j];
          mapped_type &value = elist->values[j];
          size_t idx = p->hash(key) % p->size;
          // Elementlist
          listptr next = p->buckets[idx];
          if (next == nullptr) {
            //on nil bucket, insert new ElementList
            next = std::make_shared<ListNode<Key, T>>(e_avail, PINIT, p, idx);
            next->keys[next->count] = key;
            next->values[next->count] = value;
            ++next->count;
            // install new ElementList
            p->buckets[idx] = next;
          }
          else if (next->count < EMAX) {
            next->keys[next->count] = key;
            next->values[next->count] = value;
            ++next->count;
          }
          else {
            // ElementList is full, keep doubling parent's PointList's size
            complete = false;
            break;
          }
        }
        if (!complete)
          break;
      }
    }
    if (i == cur->size)
      done = true;
  }

  return p;
}

#endif //PARALLEL_CONCURRENCY_MAP_H
