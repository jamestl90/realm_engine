# Project Brief

This document owns the project's intent and scope. Technical architecture belongs in [ARCHITECTURE.md](ARCHITECTURE.md).

## Product Intent

Build a reusable 2D game engine that can support "Rogue Farm" and future game prototypes without baking game-specific assumptions into engine subsystems.

## Scope

- Engine code, reusable runtime systems, and shared tools belong in this repository.
- Game-specific content, assets, mechanics, and release packaging should move to separate application/game repositories once they outgrow examples or prototypes.
- In-repository demos should exist to validate engine systems, not to become the primary home for a full game.

## Priorities

- Preserve engine reusability.
- Keep data and code ownership clear between engine systems and game logic.
- Prefer technical decisions that make future games easier to build, test, and maintain.
- Keep documentation non-overlapping: update the document that owns the changed fact.
