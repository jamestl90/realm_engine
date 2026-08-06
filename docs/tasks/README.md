# Task Workflow

This folder tracks engine work across its full lifecycle, including queued, active, testing, and completed work. Each task should live as a single Markdown file and move through the status folders as work progresses:

- `todo`: queued work that has not started.
- `inprogress`: active work.
- `testing`: implemented work that needs verification.
- `complete`: finished work.

Task files are planning records, not a branch policy. Small tasks can be completed directly with focused commits, and related small tasks can be grouped when they naturally belong together.

Create a Git branch when the task is large enough to justify isolation, such as work expected to take multiple days, require review, touch broad engine surfaces, or involve non-trivial merge risk.

If a task clearly requires a branch, include that in the task file with a short reason. Do not add branch metadata by default.

When starting work, move the task file from `todo` to `inprogress`. When implementation is ready, move it to `testing`. After verification and any commit or merge housekeeping, move it to `complete`.
