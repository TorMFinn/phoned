#include <iostream>

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "usage: phonectl <command> <args..>" << std::endl;
    return -1;
  }

  return 0;
}
