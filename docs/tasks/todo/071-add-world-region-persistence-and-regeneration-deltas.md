# Add World Region Persistence And Regeneration Deltas

Status: todo
Area: World / Persistence

## Goal

Persist world-region identity and modifications without treating fully generated base terrain as mutable application state.

## Context

Deterministic regions can usually be regenerated from their world identity, generator version, address, and settings. Saves should distinguish that reproducible base from durable player or simulation changes and must detect incompatible generation contracts.

## Dependencies

- Task 068: stable world and region identity.
- Task 069: deterministic local terrain contract.
- Task 070: resident-region ownership and unload lifecycle.

## Acceptance Criteria

- Define a versioned persisted region record containing world identity, region address, generator identity, and modification data.
- Define which changes are stored as deltas and when a full snapshot is required.
- Regenerate compatible base terrain and apply validated deltas in a deterministic order.
- Reject or explicitly migrate incompatible generator versions rather than silently applying deltas to different terrain.
- Integrate save/load hooks with region activation and unloading without coupling storage to rendering.
- Keep application-specific payloads behind an extensible, versioned boundary.
- Add round-trip, corruption, wrong-region, wrong-world, version-mismatch, unchanged-region, and deterministic-reconstruction tests.
- Document save ownership, atomicity expectations, and failure behavior.

## Branch

Use a dedicated branch. Persistence introduces durable formats and compatibility commitments across procgen and world lifecycle modules.
