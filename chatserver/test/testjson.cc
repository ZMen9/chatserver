#include <iostream>
#include "json.hpp"
#include <string>
#include <vector>
using namespace std;
using json = nlohmann::json;

int main() {
  vector<string> vec;
  
  json js["offlinemsg"] = vec;
  vector<string> vec2 = js["offlinemsg"]
  for (string& msg : msg_vec) {
    json js2 = json::parse(msg);
    cout << js2["msg_id"] << endl;
  }
  return 0;
}