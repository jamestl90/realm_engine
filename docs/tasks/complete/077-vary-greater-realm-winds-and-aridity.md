# Vary Greater-Realm Winds And Aridity

Status: complete
Priority: high
Area: Procgen / Greater Realm Climate

## Goal

Remove the fixed left-to-right biome cascade and allow deterministic realm seeds to range from humid worlds with little desert to arid worlds with substantial desert.

## Context

Task 075's first precipitation pass used one global west-to-east prevailing wind. Visual review after Task 076 showed that this consistently placed wet biomes upwind and deserts downwind, while the neutral precipitation scale left desert overrepresented on the representative map. A climatological normal needs broader circulation and realm-level variation rather than one permanent airflow.

## Dependencies

- Task 074: fixed-scale temperature normals and climate identity.
- Task 075: directional moisture transport, lift, and rain shadow.
- Task 076: application-driven biome rules and representative distribution test.

## Acceptance Criteria

- Replace the single default west-to-east pass with smooth latitude-dependent circulation that changes direction between tropical and mid-latitude bands.
- Blend a weaker opposing transport component so climate normals are not equivalent to one permanent weather event.
- Retain an explicit validated prevailing-wind setting as a rotation offset for authored world orientation.
- Add optional deterministic seed-driven realm wetness that can produce both low-desert and high-desert generations while preserving an exact neutral mode.
- Avoid abrupt seams at the equator and wind-band transitions.
- Preserve fixed `0..1` precipitation semantics, water-source distinctions, orographic lift, rain shadow, terrain immutability, and climate/biome regeneration ownership.
- Add tests proving wind direction differs by latitude, seed wetness is deterministic and neutralizable, and representative seeds span meaningfully different desert coverage.
- Retain a legible default sandbox biome distribution and update procgen documentation.

## Notes

This task continues on the Task 075/076 branch because it directly corrects visual evaluation of those unmerged climate and biome changes.

## Implementation Decisions

- Replaced the default global west-to-east pass with four deterministic diagonal transport passes representing opposing tropical and mid-latitude circulation in both hemispheres.
- Blended hemispheres smoothly across `-8..+8` degrees latitude and tropical/mid-latitude behavior across `25..40` degrees absolute latitude. A `0.20` opposing component keeps the climatological normal from behaving like one permanent weather event.
- Retained `prevailing_wind_degrees` as a rotation offset for the complete circulation pattern. `latitude_wind_band_strength = 0` exactly selects the prior single authored direction.
- Added a domain-separated deterministic precipitation character. Full seed variation spans effective wetness scale `0.70..1.75` and inversely varies transport loss; variation `0` returns exact neutral scale values of `1`.
- Stored effective precipitation character in the versioned climate output and its biome-source fingerprint. New settings invalidate only precipitation and dependent biome assignment.
- Preserved the directional source, lift, shadow, fixed-scale, and terrain-immutability behavior inside each reusable transport pass.

## Verification

- Added climate tests for latitude-dependent coast influence, deterministic seed character, exact neutral mode, new setting validation, and compatibility with the explicit single-wind mode.
- Added a `256x192` sandbox sweep across 25 representative seeds. Current measured desert coverage ranges from `0%` (seed `8`) to about `45.4%` (seed `7`), while seed `8675309` remains about `39.6%` desert.
- Focused climate and biome test executables passed.
- Full `ctest --test-dir out/build/debug-with-tests --output-on-failure` passed, 16/16.
- `scripts/build.ps1 -Preset debug-no-tests` and `scripts/build.ps1 -Preset release-no-tests` passed.
- `git diff --check` passed; Git reported only the repository's LF-to-CRLF conversion warnings.

## Commit Message

`fix(procgen): vary greater-realm circulation and aridity`
