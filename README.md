# 2gether

[![Core Build and Test](https://github.com/layzer26/2gether/actions/workflows/core.yml/badge.svg)](https://github.com/layzer26/2gether/actions/workflows/core.yml)

2gether is a lightweight, offline-first family hub for shared calendars, household tasks, and budgets. It is designed for low-resource environments where bandwidth and device capacity are limited. The system supports role-based permissions to ensure sensitive data is protected while keeping collaboration simple.

## Overview

The system consists of two main layers:

- **Core (C++17)**  
  Provides the business logic, event store, and data persistence (SQLite). The core is offline-first and supports delta-based synchronisation for future cloud integration.

- **Android Client (Kotlin, Jetpack Compose)**  
  A minimal user interface for creating and viewing events, tasks, and budgets. The Android app communicates with the C++ core through JNI bindings.

This repository is intended as a demonstration of clean architecture, team-ready practices, and scalable design. It is structured as a monorepo to keep the core and Android client in sync.

- For more details, see [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## Features (MVP)

- Household creation and member roles (Owner, Adult, Teen, Child, Guest).
- Shared calendars with recurring events.
- Task and chore management with assignments, due dates, and completion tracking.
- Budget categories with envelopes and monthly limits.
- Role-based visibility and permissions.
- Offline-first persistence using SQLite.
- Unit tests for the core C++ logic.
- Continuous Integration via GitHub Actions.
- Integration with google maps estimated travel times based on current traffic and calender event time

See [docs/PERMISSIONS.md](docs/PERMISSIONS.md) for details on roles and visibility.


## Technology Stack

- **C++17** for core logic.
- **SQLite3** for on-device persistence.
- **CMake + Ninja** for builds.
- **GoogleTest** for unit testing.
- **Kotlin / Jetpack Compose** for Android UI.
- **JNI** for C++/Kotlin interoperability.
- **GitHub Actions** for CI/CD.

## Repository Structure

├─ core/                     # C++ core (event store, business logic)
│  ├─ include/core/          # Public headers
│  ├─ src/                   # Implementation
│  └─ tests/                 # Unit tests
├─ android/                  # Android client (Kotlin + JNI stubs)
├─ docs/                     # Architecture, permissions, roadmap
├─ .github/workflows/        # CI pipelines
├─ .gitignore
├─ .gitattributes
├─ LICENSE
└─ README.md



## Prerequisites

- CMake (>= 3.20)
- Ninja build system
- GCC/Clang (Linux/Mac) or MSYS2 MinGW64 (Windows)
- SQLite3 development libraries
- Android Studio (for building and running the client)
- JDK 21 or later

## Building the Core

```bash
cmake -S core -B core/build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build core/build
ctest --test-dir core/build --output-on-failure
````

Expected output from the test runner:

```bash
EventStore Version: 2gether_core/0.1.0
```

## Building the Android Client

1. Open the `android/` directory in Android Studio.
2. Allow Android Studio to generate or upgrade the Gradle wrapper.
3. Ensure the NDK is installed (via SDK Manager).
4. Build and run the app on an emulator or physical device.

The app currently displays the core library version via JNI.

## Roadmap

* Extend the C++ event store with support for calendar events, tasks, and budget transactions.
* Implement role-based visibility on queries.
* Add recurrence expansion for events and tasks.
* Introduce budget rollover and reporting.
* Add CSV export and minimal reporting views in the Android client.
* Prepare delta sync endpoints for cloud integration.
  
* See [docs/ROADMAP.md](docs/ROADMAP.md) for detailed milestones.

## Contribution Guidelines

* Use [Conventional Commits](https://www.conventionalcommits.org/) for commit messages.
* Create feature branches from `main` and open pull requests for review.
* Ensure that C++ code passes `clang-format` checks.
* All new functionality must include appropriate unit tests.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.





