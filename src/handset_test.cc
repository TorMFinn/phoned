#include <iostream>
#include "handset.hpp"
#include <thread>

int main(int argc, char **argv) {
     phoned::handset handset;

     while(1) {
	  std::this_thread::sleep_for(std::chrono::seconds(1));
     }

     return 0;
}
