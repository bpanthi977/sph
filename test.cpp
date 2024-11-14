#include <_types/_uint32_t.h>
#include <_types/_uint8_t.h>
#include <fstream>
#include <ios>
#include <iostream>

int main() {
  std::ofstream file("out/test.data", std::ios::binary);
  uint16_t len = 120;
  std::cout << len << " " << sizeof(len) << " " << sizeof(char) << std::endl;
  file.write(reinterpret_cast<const char *>(&len), sizeof(uint32_t));
  file.close();
  return 0;
}
