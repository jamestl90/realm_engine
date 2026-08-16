# Task Workflow

This folder tracks engine work across its full lifecycle, including parked ideas, queued work, active work, testing, and completed work. Each record should live as a single Markdown file in the folder matching its current state:

- `ideas`: worthwhile proposals that are deliberately parked and are not part of the active backlog.
- `todo`: queued work that has not started.
- `inprogress`: active work.
- `testing`: implemented work that needs verification.
- `complete`: finished work.

Task files are planning records, not a branch policy. Small tasks can be completed directly with focused commits, and related small tasks can be grouped when they naturally belong together.

Ideas do not enter the required task process until they are promoted to `todo`. When an idea becomes a real priority, review and update its scope, dependencies, and acceptance criteria before moving it into the active backlog.

## Required Process

Every engine task follows this four-step process:

1. **Create the task.** Add a single Markdown task file and place it in `todo`. Move it to `inprogress` before implementation begins.
2. **Implement the task.** Complete the scoped work and record material implementation decisions in the task file.
3. **Add and run testing.** Add tests appropriate to the change, run the relevant verification, and record the results. Use `testing` while implemented work still awaits verification; move the task to `complete` once verification succeeds and no required work remains.
4. **Provide a commit message.** Supply a commit message that covers every change intended for that commit. If the work should be split into multiple commits, provide a message for each complete scope.

The task file and this README are the canonical records for task-specific decisions and workflow rules respectively. Do not duplicate the workflow in other documentation.

Create a Git branch when the task is large enough to justify isolation, such as work expected to take multiple days, require review, touch broad engine surfaces, or involve non-trivial merge risk.

If a task clearly requires a branch, include that in the task file with a short reason. Do not add branch metadata by default.
