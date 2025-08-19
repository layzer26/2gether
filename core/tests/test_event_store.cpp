#include "core/EventStore.h"
#include <iostream>
#include <filesystem>
#include <chrono>

using together::EventStore;
using together::DeltaEvent;
using namespace std;
static int fail(const string& msg) {
  cerr << "TEST FAIL: " << msg << endl;
  return 1;
}

int main() {
  // keep DB inside the build tree
  filesystem::create_directories("tmp");

  EventStore store;
  if (auto err = store.open("tmp/test.db"); !err.empty()) {
    return fail("open: " + err);
  }

  DeltaEvent ev;
  ev.entity_type = "task";
  ev.entity_id   = "t1";
  ev.op          = "upsert";
  ev.payload     = R"({"title":"Do dishes","points":3})";
  ev.ts          = chrono::duration_cast<chrono::milliseconds>(
                     chrono::system_clock::now().time_since_epoch()).count();

  long long seq = store.append(ev);
  if (seq < 1) return fail("append returned " + to_string(seq));

  string qerr;
  auto events = store.since(0, qerr);
  if (!qerr.empty()) return fail("since error: " + qerr);
  if (events.empty()) return fail("no events returned");

  const auto& last = events.back();
  if (last.seq != seq) return fail("seq mismatch");
  if (last.entity_type != "task") return fail("entity_type mismatch");
  if (last.entity_id != "t1") return fail("entity_id mismatch");
  if (last.op != "upsert") return fail("op mismatch");
  if (last.payload.find("Do dishes") == string::npos)
    return fail("payload content mismatch");

  cout << "OK: appended seq=" << seq
            << " version=" << EventStore::version() << endl;
  return 0;
}
