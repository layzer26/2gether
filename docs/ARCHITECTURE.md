# Architecture

## Introduction

2gether is designed as an offline-first, modular system for family collaboration. The architecture separates business logic, data persistence, and user interface concerns, ensuring scalability and maintainability. This document provides an overview of the core components, data flow, and design principles.

## Core Components

### C++ Core Library

- **Purpose:**  
  Provides all business logic and persistence in a lightweight, platform-independent library.
  
- **Responsibilities:**  
  - Maintain an append-only `event_log` for all changes.  
  - Persist domain entities (households, members, events, tasks, budgets) in SQLite.  
  - Enforce role-based visibility and permissions.  
  - Provide APIs for recurrence, budgeting, and reporting.  

- **Technologies:**  
  - C++17  
  - SQLite3  
  - CMake + Ninja for builds  
  - GoogleTest for unit testing  

- **Interfaces:**  
  - JNI bridge for Android client  
  - Future WebAssembly build for web client  

### Android Client

- **Purpose:**  
  Provides a user interface for household members on low-cost Android devices.  

- **Responsibilities:**  
  - Display household calendars, tasks, and budgets.  
  - Capture user input and forward it to the C++ core.  
  - Provide role-specific visibility (Owner, Adult, Teen, Child, Guest).  
  - Handle local notifications (task due dates, event reminders).  

- **Technologies:**  
  - Kotlin  
  - Jetpack Compose  
  - Android NDK (for linking to the C++ core)  

### Sync Service (Planned for V1)

- **Purpose:**  
  Enable multi-device synchronisation and optional cloud backup.  

- **Responsibilities:**  
  - Accept delta payloads from devices.  
  - Merge event logs and resolve conflicts using last-writer-wins semantics.  
  - Deliver encrypted updates to household members.  

- **Technologies (proposed):**  
  - Go or Rust for backend  
  - PostgreSQL for persistence  
  - REST/gRPC APIs  

## Data Flow

1. **User Action**: A member creates or updates an item (event, task, transaction).  
2. **JNI Bridge**: The Android client passes the request to the C++ core.  
3. **Core Processing**:  
   - Validate the action based on the member’s role.  
   - Append the change to `event_log`.  
   - Update the relevant entity table in SQLite.  
4. **UI Update**: The Android client queries the C++ core for the latest state and updates the view.  
5. **Sync (future)**: When online, a delta payload is generated from `event_log` and sent to the sync service.  

## Permissions Model

Roles define the default scope of access across modules:

| Role   | Calendars                        | Tasks                              | Budgets                          |
|--------|----------------------------------|------------------------------------|----------------------------------|
| Owner  | Full control, manage members     | Full                               | Full incl. sensitive             |
| Adult  | Create/update own + shared       | Full                               | View non-sensitive, add expenses |
| Teen   | View family + own                | Manage own, mark done              | View allowance only              |
| Child  | View "Kids" events only          | Mark assigned tasks done           | No access                        |
| Guest  | View events shared with guest    | No tasks                           | No budgets                       |

Per-item overrides are supported using visibility tags (`family`, `adults_only`, `kids`, `owner_only`, `custom_list`).

## Design Principles

- **Offline-first:** All functionality is available without internet access.  
- **Append-only event log:** Ensures auditable history and simple delta sync.  
- **Cross-platform core:** Business logic implemented once in C++ and reused across clients.  
- **Separation of concerns:** Clear distinction between core logic, UI, and sync.  
- **Scalability:** Architecture supports extension to multiple households and optional cloud services.  

## Future Enhancements

- Export to CSV and PDF for budgets and reports.  
- Shared shopping lists linked to budget categories.  
- Low-data reminders via WhatsApp/SMS.  
- Web client via WebAssembly build of the core library.  
- Admin dashboards for schools and NGOs (multi-household oversight).  
