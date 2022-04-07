find_package(GTest REQUIRED)

add_executable(test_contacts
  test_contacts.cpp
)

target_link_libraries(test_contacts
  libphoned
  GTest::gtest
  )

add_executable(test_modem 
  test_modem.cpp
)

target_link_libraries(test_modem
  libphoned
  GTest::gtest
)

add_test(NAME TestModem COMMAND test_modem)
