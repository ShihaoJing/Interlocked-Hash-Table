//
// Created by Shihao Jing on 10/28/17.
//

#include <iostream>
#include "map.h"

int main() {
  Map<int, int> m;
  m.Insert(1, 123);
  m.Remove(1);
  auto p = m.Lookup(1);
  if (p.first)
    std::cout << *p.second << std::endl;
  else
    std::cerr << "Key not found\n";
}