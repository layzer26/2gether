cmake_minimum_required(VERSION 3.20)
project(2gether_core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SQLite (use MSYS2 MinGW64 where sqlite3 is installed)
find_package(SQLite3 REQUIRED)

add_library(2gether_core
  src/EventStore.cpp
)
target_include_directories(2gether_core PUBLIC include)
target_link_libraries(2gether_core PUBLIC SQLite::SQLite3)

enable_testing()
add_executable(core_tests tests/test_event_store.cpp)
target_link_libraries(core_tests PRIVATE 2gether_core)
add_test(NAME ScenarioA COMMAND core_tests --case A)
add_test(NAME ScenarioB COMMAND core_tests --case B)
add_test(NAME ScenarioC COMMAND core_tests --case C)
add_test(NAME ScenarioD COMMAND core_tests --case D)
add_test(NAME ScenarioE COMMAND core_tests --case E)


add_test(NAME AllScenarios COMMAND core_tests)