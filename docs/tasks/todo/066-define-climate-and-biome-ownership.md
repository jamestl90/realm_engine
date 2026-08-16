# Define Climate And Biome Ownership

Status: todo
Priority: high
Area: Procgen / World Simulation Architecture

## Goal

Define the engine boundary between stable generated environmental tendencies, biome assignment, and time-varying runtime weather before adding any new climate or biome fields.

## Context

Task 028 generated rainfall, humidity, and moisture during map generation. Task 039 removed those fields because weather events, runoff, and active discharge belong to runtime simulation. Task 040 nevertheless depends on biome and world-region data that can describe local weather tendencies.

The project therefore needs an explicit contract for any stable climate normals or biome inputs, including whether the reusable engine exports generic environmental factors while applications supply biome definitions. This decision must preserve the established rule that procgen does not predetermine weather events.

## Dependencies

- Task 039: separation of generated drainage from runtime weather.
- Task 064: complete stable water classification.
- Task 065: watershed metadata available as a possible environmental input.

## Acceptance Criteria

- Decide whether stable climate normals or environmental tendency fields belong in procgen, world simulation, or application data.
- Decide whether biome labels are engine-owned, application-owned, or produced by an engine classifier from application-supplied rules.
- Define the minimum reusable inputs and outputs without reintroducing generated precipitation events, transient humidity, soil moisture, runoff, or discharge.
- Define how elevation, latitude, coast distance, terrain form, inland water, and watersheds may influence stable tendencies.
- Define determinism, serialization, regeneration, and streamed-region ownership expectations.
- Record the dependency direction between greater-realm generation, biome data, world regions, and Task 040 runtime weather.
- Update `docs/ARCHITECTURE.md`, `docs/PROCGEN.md`, and Task 040 where the decision changes their contracts.
- Create focused implementation tasks for the approved climate-normal and biome slices; do not implement those slices in this task.

## Notes

This is a decision and task-breakdown record. It exists to prevent a repeat of the Task 028/039 ownership reversal.

No branch is required because this task changes documentation and backlog scope only.
