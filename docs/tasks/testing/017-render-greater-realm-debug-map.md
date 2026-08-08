# Render Greater Realm Debug Map

Status: testing
Area: Procgen / Rendering
Branch: `proc-gen`
Branch reason: This is part of the broader procedural generation feature stream and depends on the greater realm generator foundation.

## Goal

Render a generated greater realm map so the broad terrain shape can be judged visually before adding rivers, biomes, or world management.

## Context

The first generator now produces map data, but the main question at this stage is visual: whether the generated continents, islands, coastlines, mountains, highlands, and lowlands feel believable.

The debug rendering path should be temporary/development-facing and should not become part of release runtime code.

## Acceptance Criteria

- Generate a `GreaterRealmMap` from fixed debug settings.
- Convert terrain forms into a simple coloured debug texture.
- Render the debug texture through the existing sprite/ECS rendering path.
- Keep the debug visualization compile-gated so it does not compile into release builds by default.
- Verify the normal game target still builds.
- Verify a release configuration can compile without the debug visualization enabled.

## Notes

This is visual validation, not the final terrain renderer.
