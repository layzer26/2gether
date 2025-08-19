#include "core/EventStore.h"
#include <sqlite3.h>
#include <cstring>

namespace together {

EventStore::EventStore() = default;

EventStore::~EventStore() {
  if (db_) {
    sqlite3_close(db_);
    db_ = nullptr;
  }
}

const char* EventStore::version() {
  return "2gether_core/0.2.0";
}

string EventStore::open(const string& db_path) {
  if (db_) return {}; // already open

  if (sqlite3_open(db_path.c_str(), &db_) != SQLITE_OK) {
    string err = sqlite3_errmsg(db_);
    sqlite3_close(db_);
    db_ = nullptr;
    return "sqlite open failed: " + err;
  }

  string ierr = initSchema();
  if (!ierr.empty()) return ierr;

  // Use WAL mode for better durability
  char* errmsg = nullptr;
  sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, &errmsg);
  if (errmsg) sqlite3_free(errmsg);

  return {};
}

string EventStore::initSchema() {
  const char* ddl = R"SQL(
    CREATE TABLE IF NOT EXISTS event_log(
      seq INTEGER PRIMARY KEY AUTOINCREMENT,
      entity_type TEXT NOT NULL,
      entity_id   TEXT NOT NULL,
      op          TEXT NOT NULL,     -- 'upsert' | 'delete'
      payload_blob BLOB NOT NULL,    -- JSON now; protobuf later
      ts          INTEGER NOT NULL   -- epoch millis
    );
    CREATE INDEX IF NOT EXISTS idx_event_log_seq ON event_log(seq);
  )SQL";

  char* errmsg = nullptr;
  int rc = sqlite3_exec(db_, ddl, nullptr, nullptr, &errmsg);
  if (rc != SQLITE_OK) {
    string err = errmsg ? errmsg : "unknown";
    if (errmsg) sqlite3_free(errmsg);
    return "schema creation failed: " + err;
  }
  return {};
}

long long EventStore::append(const DeltaEvent& ev) {
  if (!db_) return -1;

  const char* sql =
      "INSERT INTO event_log(entity_type, entity_id, op, payload_blob, ts) "
      "VALUES(?,?,?,?,?)";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    return -2;
  }

  sqlite3_bind_text (stmt, 1, ev.entity_type.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text (stmt, 2, ev.entity_id.c_str(),   -1, SQLITE_TRANSIENT);
  sqlite3_bind_text (stmt, 3, ev.op.c_str(),          -1, SQLITE_TRANSIENT);
  sqlite3_bind_blob (stmt, 4, ev.payload.data(), (int)ev.payload.size(), SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 5, (sqlite3_int64)ev.ts);

  int step_rc = sqlite3_step(stmt);
  if (step_rc != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    return -3;
  }
  long long new_seq = (long long)sqlite3_last_insert_rowid(db_);
  sqlite3_finalize(stmt);
  return new_seq;
}

vector<DeltaEvent> EventStore::since(long long since_seq, string& out_error) const {
  vector<DeltaEvent> out;
  out_error.clear();

  if (!db_) {
    out_error = "database not open";
    return out;
  }

  const char* sql =
      "SELECT seq, entity_type, entity_id, op, payload_blob, ts "
      "FROM event_log WHERE seq > ? ORDER BY seq ASC";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    out_error = "prepare failed";
    return out;
  }

  sqlite3_bind_int64(stmt, 1, (sqlite3_int64)since_seq);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    DeltaEvent e;
    e.seq         = sqlite3_column_int64(stmt, 0);
    e.entity_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    e.entity_id   = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    e.op          = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

    const void* blob = sqlite3_column_blob(stmt, 4);
    int bsz = sqlite3_column_bytes(stmt, 4);
    if (blob && bsz > 0) {
      e.payload.assign((const char*)blob, (const char*)blob + bsz);
    }

    e.ts = sqlite3_column_int64(stmt, 5);
    out.push_back(move(e));
  }

  sqlite3_finalize(stmt);
  return out;
}

} // namespace together
