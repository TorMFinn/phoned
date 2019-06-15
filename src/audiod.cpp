/**
 * Handles audio transfer to the modem and
 * dialtone playback
 */

#include "skimpbuffers/BusConnection.hpp"

using namespace skimpbuffers;

int main() {
  auto bus = BusConnection::Create("phone");

  bus->AddDataSink([](const skimpbuffers::Data& data) {
    if (data.payload == "handset_up") {
      // start dialtone
    } else {
      // stop dialtone
    }
  });

  return 0;
}
