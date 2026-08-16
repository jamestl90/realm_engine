# Document Four-Step Task Workflow

Status: complete
Area: Documentation / Task Management

## Goal

Make the required four-step engine task process explicit in the canonical task workflow documentation.

## Acceptance Criteria

- Document task creation, implementation, testing, and commit-message delivery as required steps.
- Connect each step to the existing task status folders without duplicating status definitions.
- Preserve the existing branch policy.
- Confirm the workflow is stated in one canonical location.

## Implementation

- Added a `Required Process` section to `docs/tasks/README.md`.
- Defined task creation, implementation, testing, and commit-message delivery as the required four steps.
- Connected the steps to the existing task status folders and retained the existing branch policy.
- Identified the README as the sole canonical workflow source.

## Verification

- A documentation search confirms the four-step workflow is defined only in `docs/tasks/README.md`.
- Markdown changes pass `git diff --check`.
