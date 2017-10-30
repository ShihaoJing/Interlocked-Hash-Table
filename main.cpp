//
// Created by Shihao Jing on 10/28/17.
//

#include <iostream>
#include <string>
#include "map.h"

int main() {
  Map<std::string, int> m;
  m.Insert("abc", 123);
  //m.Remove("abc");
  auto p = m.Lookup("abc");
  if (p.first)
    std::cout << *p.second << std::endl;
  else
    std::cerr << "Key not found\n";
}