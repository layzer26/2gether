#include "core/EventStore.h"
#include <sqlite3.h>

#include <cstring>
#include <utility>
#include <string>
#include <vector>

using std::string;
using std::vector;

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

  // WAL improves durability/concurrency
  char* errmsg = nullptr;
  sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, &errmsg);
  if (errmsg) sqlite3_free(errmsg);

  return {};
}

string EventStore::initSchema() {
  const char* ddl = R"SQL(
    -- Event log: append-only history
    CREATE TABLE IF NOT EXISTS event_log(
      seq INTEGER PRIMARY KEY AUTOINCREMENT,
      entity_type TEXT NOT NULL,
      entity_id   TEXT NOT NULL,
      op          TEXT NOT NULL,     -- 'upsert' | 'delete'
      payload_blob BLOB NOT NULL,    -- JSON now; protobuf later
      ts          INTEGER NOT NULL   -- epoch millis
    );
    CREATE INDEX IF NOT EXISTS idx_event_log_seq ON event_log(seq);

    -- Task table: current state of tasks
    CREATE TABLE IF NOT EXISTS task (
      id TEXT PRIMARY KEY,
      title TEXT NOT NULL,
      assignees_csv TEXT DEFAULT '',
      due_at INTEGER DEFAULT 0,
      points INTEGER DEFAULT 0,
      status TEXT DEFAULT 'open',
      visibility_tag TEXT DEFAULT 'family',
      updated_at INTEGER NOT NULL
    );
    CREATE INDEX IF NOT EXISTS idx_task_status ON task(status);
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
    out.push_back(std::move(e));
  }

  sqlite3_finalize(stmt);
  return out;
}

long long EventStore::upsertTask(const string& id,
                                 const string& title,
                                 const string& assignees_csv,
                                 long long due_at,
                                 int points,
                                 const string& status,
                                 const string& visibility_tag,
                                 long long updated_at_millis,
                                 const string& payload_json) {
  if (!db_) return -1;

  // Begin a transaction so the table write and event append are atomic
  char* errmsg = nullptr;
  if (sqlite3_exec(db_, "BEGIN IMMEDIATE;", nullptr, nullptr, &errmsg) != SQLITE_OK) {
    string err = errmsg ? errmsg : "begin failed";
    if (errmsg) sqlite3_free(errmsg);
    return -2;
  }

  // 1) Upsert task row
  const char* sql_task =
      "INSERT INTO task(id, title, assignees_csv, due_at, points, status, visibility_tag, updated_at) "
      "VALUES(?,?,?,?,?,?,?,?) "
      "ON CONFLICT(id) DO UPDATE SET "
      "  title=excluded.title, "
      "  assignees_csv=excluded.assignees_csv, "
      "  due_at=excluded.due_at, "
      "  points=excluded.points, "
      "  status=excluded.status, "
      "  visibility_tag=excluded.visibility_tag, "
      "  updated_at=excluded.updated_at";

  sqlite3_stmt* st_task = nullptr;
  if (sqlite3_prepare_v2(db_, sql_task, -1, &st_task, nullptr) != SQLITE_OK) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    return -3;
  }

  sqlite3_bind_text (st_task, 1, id.c_str(),            -1, SQLITE_TRANSIENT);
  sqlite3_bind_text (st_task, 2, title.c_str(),         -1, SQLITE_TRANSIENT);
  sqlite3_bind_text (st_task, 3, assignees_csv.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(st_task, 4, (sqlite3_int64)due_at);
  sqlite3_bind_int  (st_task, 5, points); // <-- this was wrong before
  sqlite3_bind_text (st_task, 6, status.c_str(),        -1, SQLITE_TRANSIENT);
  sqlite3_bind_text (st_task, 7, visibility_tag.c_str(),-1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(st_task, 8, (sqlite3_int64)updated_at_millis);

  if (sqlite3_step(st_task) != SQLITE_DONE) {
    sqlite3_finalize(st_task);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    return -4;
  }
  sqlite3_finalize(st_task);

  // 2) Append corresponding event_log record
  const char* sql_ev =
      "INSERT INTO event_log(entity_type, entity_id, op, payload_blob, ts) "
      "VALUES(?,?,?,?,?)";

  sqlite3_stmt* st_ev = nullptr;
  if (sqlite3_prepare_v2(db_, sql_ev, -1, &st_ev, nullptr) != SQLITE_OK) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    return -5;
  }

  const char* entity_type = "task";
  const char* op          = "upsert";

  sqlite3_bind_text (st_ev, 1, entity_type,              -1, SQLITE_TRANSIENT);
  sqlite3_bind_text (st_ev, 2, id.c_str(),               -1, SQLITE_TRANSIENT);
  sqlite3_bind_text (st_ev, 3, op,                       -1, SQLITE_TRANSIENT);
  sqlite3_bind_blob (st_ev, 4, payload_json.data(), (int)payload_json.size(), SQLITE_TRANSIENT);
  sqlite3_bind_int64(st_ev, 5, (sqlite3_int64)updated_at_millis);

  if (sqlite3_step(st_ev) != SQLITE_DONE) {
    sqlite3_finalize(st_ev);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    return -6;
  }

  long long ev_seq = (long long)sqlite3_last_insert_rowid(db_);
  sqlite3_finalize(st_ev);

  // 3) Commit
  if (sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &errmsg) != SQLITE_OK) {
    string err = errmsg ? errmsg : "commit failed";
    if (errmsg) sqlite3_free(errmsg);
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    return -7;
  }

  return ev_seq;
}

} // namespace together
