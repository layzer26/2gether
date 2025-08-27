#include "core/EventStore.h"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

using together::DeltaEvent;
using together::EventStore;
using together::TaskRow; // changed: TaskRow is now at namespace scope

static int fail(const std::string &msg) {
  std::cerr << "TEST FAIL: " << msg << std::endl;
  return 1;
}

static int scenarioA(EventStore &store);
static int scenarioB(EventStore &store);
static int scenarioC(EventStore &store);
static int scenarioD(EventStore &store);
static int scenarioE(EventStore &store);

int main(int argc, char **argv) {
  std::string which;
  if (argc >= 3 && std::string(argv[1]) == "--case") {
    which = argv[2]; 
  }

  std::filesystem::create_directories("tmp");

  EventStore store;
  if (auto err = store.open("tmp/test.db"); !err.empty()) {
    return fail(std::string("open: ") + err);
  }

  if (!which.empty()) {
    if (which == "A")
      return scenarioA(store);
    if (which == "B")
      return scenarioB(store);
    if (which == "C")
      return scenarioC(store);
    if (which == "D")
      return scenarioD(store);
    if (which == "E")
      return scenarioE(store);
    return fail("unknown case " + which);
  }

  // default: run all
  int rc = 0;
  rc |= scenarioA(store);
  rc |= scenarioB(store);
  rc |= scenarioC(store);
  rc |= scenarioD(store);
  rc |= scenarioE(store);
  if (rc == 0) {
    std::cout << "OK: all scenarios passed\n";
  }
  return rc;
}

static int scenarioA(EventStore &store) {
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
  return 0;
}
static int scenarioB(EventStore &store) {
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
  return 0;
}
static int scenarioC(EventStore &store) {
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
  return 0;
}
static int scenarioD(EventStore &store) {
  // --- Scenario D: getTaskById returns the row we just wrote
  {
    const long long now_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
    const std::string payload =
        R"({"title":"Homework","assignees":"kid3","points":4})";
    long long ev = store.upsertTask("t_read", "Homework", "kid3", 0, 4, "open",
                                    "family", now_ms, payload);
    if (ev < 1)
      return fail("Scenario D: upsertTask failed");

    TaskRow row; // changed: use namespace-level TaskRow
    std::string err;
    bool ok = store.getTaskId("t_read", row, err); // changed: getTaskId matches header
    if (!ok)
      return fail(std::string("Scenario D: getTaskId failed: ") + err);

    if (row.title != "Homework")
      return fail("Scenario D: title mismatch");
    if (row.assignees_csv != "kid3")
      return fail("Scenario D: assignees mismatch");
    if (row.points != 4)
      return fail("Scenario D: points mismatch");
    if (row.status != "open")
      return fail("Scenario D: status mismatch");
  }
  return 0;
}
static int scenarioE(EventStore &store) {
  // --- Scenario E: listTasks returns recent items, filterable by status
  {
    const long long now_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();

    // Seed 
    store.upsertTask("t_l1", "Pack lunch", "kid1", 0, 1, "open", "family",
                     now_ms, R"({})");
    store.upsertTask("t_l2", "Wash car", "dad", 0, 2, "done", "family",
                     now_ms + 1, R"({})");
    store.upsertTask("t_l3", "Math drills", "kid2", 0, 3, "open", "family",
                     now_ms + 2, R"({})");

    std::string err;
    auto all = store.listTasks("", 10, 0, err);
    if (!err.empty())
      return fail(std::string("Scenario E: list all error: ") + err);
    if (all.size() < 3)
      return fail("Scenario E: expected >=3 tasks in 'all'");

    auto open = store.listTasks("open", 10, 0, err);
    if (!err.empty())
      return fail(std::string("Scenario E: list open error: ") + err);
    bool found_open = false;
    for (auto &r : open)
      if (r.status == "open") {
        found_open = true;
        break;
      }
    if (!found_open)
      return fail("Scenario E: expected at least one 'open' task");
  }
  return 0;
}

