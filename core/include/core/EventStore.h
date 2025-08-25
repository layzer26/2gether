#pragma once
#include <string>
#include <vector>

struct sqlite3; // forward-declare

namespace together {

// Minimal event shape for now (payload kept as JSON string)
struct DeltaEvent {
  long long seq = 0;        // assigned by DB
  std::string entity_type;  // "task", "event", "budget_tx", ...
  std::string entity_id;    // caller id (e.g., "t1")
  std::string op;           // "upsert" | "delete"
  std::string payload;      // JSON now (protobuf later)
  long long ts = 0;         // epoch millis
};

class EventStore {
public:
  EventStore();
  ~EventStore();

  EventStore(const EventStore&) = delete;
  EventStore& operator=(const EventStore&) = delete;

  // Open or create the database file; ensures schema exists.
  // Returns empty string on success; otherwise error message.
  std::string open(const std::string& db_path);

  // Append one event; returns new seq (>=1) or negative error code.
  long long append(const DeltaEvent& ev);

  // Return events with seq > since_seq (ascending).
  // On error, returns empty vector and sets out_error.
  std::vector<DeltaEvent> since(long long since_seq, std::string& out_error) const;

  // Insert or update a task row and append a matching event_log record.
  // Returns the event_log seq (>=1) on success; negative on error.
  long long upsertTask(const std::string& id,
                       const std::string& title,
                       const std::string& assignees_csv,
                       long long due_at,
                       int points,
                       const std::string& status,
                       const std::string& visibility_tag,
                       long long updated_at_millis,
                       const std::string& payload_json);

  static const char* version();

private:
  sqlite3* db_ = nullptr;

  // Create tables/indexes. Empty string on success, else error message.
  std::string initSchema();
};

} // namespace together
