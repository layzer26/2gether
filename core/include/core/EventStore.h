#pragma once
#include <string>
#include <vector>

struct sqlite3; // forward declaration (we include the header in the .cpp)
using namespace std;
namespace together {

// Minimal event shape for now (payload kept as JSON string)
struct DeltaEvent {
  long long seq = 0;        // assigned by DB
  string entity_type;  // "task", "event", "budget_tx", ...
  string entity_id;    // caller id (e.g., "t1")
  string op;           // "upsert" | "delete"
  string payload;      // JSON now (protobuf later)
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
  string open(const string& db_path);

  // Append one event; returns new seq (>=1) or negative error code.
  long long append(const DeltaEvent& ev);

  // Return events with seq > since_seq (ascending).
  // On error, returns empty vector and sets out_error.
  vector<DeltaEvent> since(long long since_seq, string& out_error) const;

  static const char* version();

private:
  sqlite3* db_ = nullptr;

  // Create tables/indexes. Empty string on success, else error message.
  string initSchema();
};

} // namespace together
