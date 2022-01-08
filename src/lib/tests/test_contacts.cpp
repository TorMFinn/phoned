#include <gtest/gtest.h>
#include <phoned/contacts.hpp>

class StorageTests : public ::testing::Test {
protected:
  void SetUp() {
    m_storage.Open("/tmp/contact_storage.db");
  }

  phoned::ContactStorage m_storage;
};

TEST_F(StorageTests, test_storage) {

}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
