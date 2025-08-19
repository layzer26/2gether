# Roadmap

## Purpose

This roadmap outlines the planned delivery milestones for the 2gether project. It distinguishes between the Minimum Viable Product (MVP), immediate extensions, and longer-term development goals. Timelines are indicative and assume incremental delivery.

---

## Phase 1: MVP (Weeks 1–8)

**Objective:** Deliver a working offline-first family hub with core functionality.  
**Scope:**

- **Core (C++):**
  - Event store with append-only `event_log`.
  - Basic schema for households, members, tasks, budgets.
  - Simple persistence using SQLite.
  - Unit tests for event store operations.

- **Android Client (Kotlin/Compose):**
  - Household creation and member roles.
  - Task list (create, assign, mark done).
  - Budget categories with simple expense tracking.
  - Display of app/core version (via JNI) for verification.

- **Permissions:**
  - Enforce role-based visibility (Owner, Adult, Teen, Child, Guest).
  - Support `visibility_tag` overrides on tasks/events.

- **Infrastructure:**
  - GitHub Actions workflow for C++ builds and tests.
  - Repository hygiene (.gitignore, .gitattributes, docs).

**Deliverable:**  
A lightweight Android app running fully offline, supported by a tested C++ core, demonstrating tasks and budgets with role-based visibility.

---

## Phase 2: Immediate Extensions (V1)

**Objective:** Expand core functionality and reporting.  
**Scope:**

- **Core:**
  - Recurrence engine for tasks and events.
  - Budget rollover (carryover and monthly limits).
  - CSV export of budget data.

- **Android Client:**
  - Monthly budget summary view.
  - Calendar view with recurring events.
  - Simple reporting dashboards (chore completion, budget usage).

- **Permissions:**
  - Read-only allowance view for minors.

- **Infrastructure:**
  - Extended CI to include Android build/linting.
  - Automated formatting checks (clang-format, ktlint).

**Deliverable:**  
A feature-complete V1 supporting households with basic calendars, tasks, and budgets, plus export and reporting.

---

## Phase 3: Advanced Features (Post-V1)

**Objective:** Introduce collaboration and cloud capabilities.  
**Scope:**

- **Core/Sync Service:**
  - Delta sync with compressed payloads (<20 KB typical).
  - Conflict resolution using last-writer-wins and semantic merges.
  - Household key rotation on membership change.

- **Android Client:**
  - Shared shopping lists linked to budgets.
  - Low-data WhatsApp/SMS reminders.
  - Multiple household support.

- **Backend (optional SaaS):**
  - REST/gRPC service for sync and backup.
  - Postgres storage with multi-tenant isolation.
  - Admin dashboards for schools/NGOs.

**Deliverable:**  
Cloud-enabled, multi-household platform with reporting and reminders.

---

## Phase 4: Long-Term Goals

- Web client built on WebAssembly using the C++ core.
- Advanced analytics: task completion rates, budget forecasting.
- Premium subscription tiers (cloud sync, multiple households, PDF reports).
- Support for integrations with schools, NGOs, or local government initiatives.

---

## Tracking

- Milestones are managed via GitHub Projects/Issues.
- Each feature should be delivered through a dedicated branch and pull request.
- Documentation to be updated continuously (`ARCHITECTURE.md`, `PERMISSIONS.md`, API references).
