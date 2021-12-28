#include <utility>
#include <iostream>
#include <map>
#include <string>
using std::map;
using std::pair;
using std::string;
using std::cout;
using std::endl;

int main() {
  pair<int, string> p1{2,"5"};
  pair<int,string> p2(3,"9");
  p1.second = "8";
  map<int,string> m;
  m.insert({3,"local"});
  m.insert({4,"yes"});
  auto it = m.find(4);
  if (it != m.end()) {
    it->second = "no";
  }
  cout << p1.second << endl;
  cout << m[4] << endl;
  return 0;
}
