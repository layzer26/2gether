# Permissions Model

## Introduction

The permissions model in 2gether is designed to provide granular access control across calendars, tasks, and budgets. Roles are assigned at the household level and can be further refined by per-item visibility tags. This ensures sensitive information is protected while keeping collaboration simple.

---

## Roles and Capabilities

| Role   | Calendars                          | Tasks                                | Budgets                                   |
|--------|------------------------------------|--------------------------------------|-------------------------------------------|
| Owner  | Full control, manage members       | Full control                         | Full control, including sensitive entries |
| Adult  | Create/update own and shared       | Full control                         | View non-sensitive, add expenses          |
| Teen   | View family and own                | Create/update own, mark done         | View allowance only                       |
| Child  | View events tagged `Kids`          | Mark assigned tasks as done          | No access                                 |
| Guest  | View events shared with guest      | No tasks                             | No access                                 |

---

## Visibility Tags

Each entity (event, task, budget entry) can carry a `visibility_tag` to override default role permissions. Supported tags include:

- `family` – visible to all household members
- `adults_only` – visible to Owner and Adult roles only
- `kids` – visible to Child and above
- `owner_only` – restricted to Owner role
- `custom_list` – restricted to a defined subset of members

Visibility tags are stored at the record level in SQLite and enforced in core queries.

---

## Enforcement

- Permissions are enforced in the **C++ core** before data is returned to the Android client.
- Default role permissions apply unless an explicit `visibility_tag` is set.
- Sensitive budget entries can be flagged with `is_sensitive = true` to restrict access further.

---

## Future Enhancements

- Allow per-module overrides (e.g., a Teen promoted to co-manage tasks).
- Support dynamic groups for `custom_list` visibility.
- Introduce audit logging for permission changes in the event log.
