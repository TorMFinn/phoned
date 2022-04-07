#include <SQLiteCpp/SQLiteCpp.h>
#include <phoned/contacts.hpp>

const std::string create_db = R"(
CREATE TABLE IF NOT EXISTS contacts (
id INTEGER PRIMARY KEY,
first_name TEXT,
last_name TEXT,
number TEXT
);
)";

namespace phoned {

struct ContactStorage::Impl {
  std::unique_ptr<SQLite::Database> db;
};

ContactStorage::ContactStorage() {}

ContactStorage::~ContactStorage() {}

bool ContactStorage::Open(const boost::filesystem::path &path) {
  m_impl->db = std::make_unique<SQLite::Database>(
      path.string(), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

  SQLite::Statement query(*m_impl->db, create_db);
  return query.executeStep();
}

Contact ContactStorage::GetByNumber(const std::string &number) {}
} // namespace phoned
