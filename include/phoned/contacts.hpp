#pragma once
#include <boost/filesystem.hpp>
#include <memory>
#include <vector>

namespace phoned {

class Contact {
public:
  Contact();
  ~Contact();

  void SetFirstName(const std::string &name);
  std::string GetFirstName() const;

  void SetLastName(const std::string &name);
  std::string GetLastName() const;

  void SetNumber(const std::string &number);
  std::string GetNumber() const;

  void SetId_(int id);
  int GetId_();

private:
  std::string m_number;
  std::string m_first_name;
  std::string m_last_name;

  int m_id;
};

using ContactList = std::vector<Contact>;

class ContactStorage {
public:
  ContactStorage();
  ~ContactStorage();

  bool Open(const boost::filesystem::path &path);
  Contact GetByNumber(const std::string &number);

  bool AddContact(const Contact &contact);
  bool RemoveContact(const Contact &contact);

  ContactList GetContacts();

private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;
};
} // namespace phoned
