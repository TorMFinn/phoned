#include <iostream>
#include <boost/filesystem.hpp>

int main(int argc, char **argv) {
   std::cout << "Hello World\n" << std::endl;

   if(boost::filesystem::exists("/mnt")) {
	   std::cout << "Mount folder exists" << std::endl;
   }
   return 0;
}
