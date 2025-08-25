#include "core/EventStore.h"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

using together::DeltaEvent;
using together::EventStore;

static int fail(const std::string &msg) {
  std::cerr << "TEST FAIL: " << msg << std::endl;
  return 1;
}

int main() {
  // CTest runs from core/build as CWD; keep DB under build/tmp/
  std::filesystem::create_directories("tmp");

  EventStore store;
  if (auto err = store.open("tmp/test.db"); !err.empty()) {
    return fail(std::string("open: ") + err);
  }

  // --- Scenario A: direct append to event_log
  {
    DeltaEvent ev;
    ev.entity_type = "task";
    ev.entity_id = "t_direct";
    ev.op = "upsert";
    ev.payload = R"({"title":"Do dishes","points":3})";
    ev.ts = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();

    long long seq = store.append(ev);
    if (seq < 1)
      return fail(std::string("append returned ") + std::to_string(seq));

    std::string qerr;
    auto events = store.since(0, qerr);
    if (!qerr.empty())
      return fail(std::string("since error: ") + qerr);
    if (events.empty())
      return fail("no events returned (scenario A)");
  }

  // --- Scenario B: upsertTask (writes task + appends event atomically)
  {
    const long long now_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();

    const std::string payload =
        R"({"title":"Sweep floor","assignees":"kid1","points":2})";

    long long ev_seq = store.upsertTask("t1",          // id
                                        "Sweep floor", // title
                                        "kid1",        // assignees_csv
                                        0,             // due_at
                                        2,             // points
                                        "open",        // status
                                        "family",      // visibility_tag
                                        now_ms,        // updated_at_millis
                                        payload        // payload_json
    );

    if (ev_seq < 1)
      return fail(std::string("upsertTask returned ") + std::to_string(ev_seq));

    std::string qerr;
    auto events = store.since(0, qerr);
    if (!qerr.empty())
      return fail(std::string("since error (scenario B): ") + qerr);
    if (events.empty())
      return fail("no events returned (scenario B)");

    const auto &last = events.back();
    if (last.seq != ev_seq)
      return fail("seq mismatch (scenario B)");
    if (last.entity_type != "task")
      return fail("entity_type mismatch (scenario B)");
    if (last.entity_id != "t1")
      return fail("entity_id mismatch (scenario B)");
    if (last.op != "upsert")
      return fail("op mismatch (scenario B)");
    if (last.payload.find("Sweep floor") == std::string::npos)
      return fail("payload content mismatch (scenario B)");
  }

  // --- Scenario C: deleteTask (soft-delete + append delete event)
  {
    // ensure a task exists to delete
    const long long now_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
    const std::string payload_upsert = R"({"title":"Test task to delete",
                                        "assignees":"kid1","points":2})";

    long long ev_upsert = store.upsertTask("t_del",               // id
                                           "Test task to delete", // title
                                           "kid1",        // assignees_csv
                                           0,             // due_at
                                           2,             // points
                                           "open",        // status
                                           "family",      // visibility_tag
                                           now_ms,        // updated_at_millis
                                           payload_upsert // payload_json
    );

    if (ev_upsert < 1)
      return fail(std::string("upsertTask (for delete) returned ") +
                  std::to_string(ev_upsert));

    // now delete it
    const std::string payload_delete = R"({"reason":"user_deleted"})";
    long long ev_del = store.deleteTask("t_del", now_ms, payload_delete);
    if (ev_del < 1)
      return fail(std::string("deleteTask returned ") + std::to_string(ev_del));

    // Verify the last event is the delete event
    std::string qerr;
    auto events = store.since(0, qerr);
    if (!qerr.empty())
      return fail(std::string("since error (scenario C): ") + qerr);
    if (events.empty())
      return fail(std::string("since error(scenario C) :") + qerr);
    if (events.empty())
      return fail("no events returned (scenario C)");

    const auto &last = events.back();
    if (last.entity_type != "task")
      return fail("entity_type mismatch (scenario C)");
    if (last.entity_id != "t_del")
      return fail("entity_id mismatch (scenario C)");
    if (last.op != "delete")
      return fail("op mismatch (scenario C)");

    // sanity check payload
    std::cout << "DEBUG last: seq=" << last.seq << " type=" << last.entity_type
              << " id=" << last.entity_id << " op=" << last.op
              << " payload=" << last.payload << std::endl;

    if (last.payload.find("user_deleted") == std::string::npos)
      return fail("payload content mismatch (scenario C)");
  }

  std::cout << "OK: upsertTask + event_log verified, version="
            << EventStore::version() << std::endl;

  return 0;
}
