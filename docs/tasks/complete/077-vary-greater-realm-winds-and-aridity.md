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
- Add deterministic broad regional flow variation so seeds do not all share the same wind geometry within a latitude band.
- Keep regional variation smooth enough to preserve coherent coastal moisture, orographic lift, and rain-shadow regions rather than adding cell-scale precipitation noise.
- Provide an exact neutral wind-variation mode that restores the latitude-band baseline without seed-driven direction changes.
- Add a cross-seed regression proving that no single map side or compass direction consistently owns the strongest coastal moisture influence.

## Notes

This task continues on the Task 075/076 branch because it directly corrects visual evaluation of those unmerged climate and biome changes.

The task was reopened after review found that opposing latitude bands still gave every seed the same circulation geometry. The first implementation remains the neutral baseline; completion now requires genuine seed-to-seed regional orientation variation.

## Implementation Decisions

- Replaced the default global west-to-east pass with four deterministic diagonal transport passes representing opposing tropical and mid-latitude circulation in both hemispheres.
- Blended hemispheres smoothly across `-8..+8` degrees latitude and tropical/mid-latitude behavior across `25..40` degrees absolute latitude. A `0.20` opposing component keeps the climatological normal from behaving like one permanent weather event.
- Retained `prevailing_wind_degrees` as a rotation offset for the complete circulation pattern. `latitude_wind_band_strength = 0` exactly selects the prior single authored direction.
- Added a domain-separated deterministic precipitation character. Full seed variation spans effective wetness scale `0.70..1.75` and inversely varies transport loss; variation `0` returns exact neutral scale values of `1`.
- Stored effective precipitation character in the versioned climate output and its biome-source fingerprint. New settings invalidate only precipitation and dependent biome assignment.
- Preserved the directional source, lift, shadow, fixed-scale, and terrain-immutability behavior inside each reusable transport pass.
- Extended the reopened task with a domain-separated wind character: full-compass global rotation, a small latitude-band shift, independent hemisphere offsets, and a regional-strength scale all derive deterministically from the realm seed.
- Reduced the latitude template's default strength to `0.35`, making it an anchor rather than an absolute pattern while retaining explicit `0..1` authoring control.
- Built an eight-direction transport atlas at `45`-degree intervals. A low-frequency seed field smoothly selects between adjacent full transport results, so regional direction changes preserve water pickup, mountain lift, and rain shadows rather than perturbing final precipitation values.
- Added `wind_seed_variation`; zero returns the exact seed-neutral latitude baseline under the current settings, one enables the complete regional character, and intermediate values blend the two outputs.
- Stored effective wind character in climate identity and bumped the climate output version. Wind settings invalidate precipitation and dependent biome assignment only.

## Verification

- Added climate tests for latitude-dependent coast influence, deterministic seed character, exact neutral mode, new setting validation, and compatibility with the explicit single-wind mode.
- Added a `256x192` sandbox sweep across 25 representative seeds. With regional winds, current desert coverage ranges from `0%` (seed `8`) to about `63.4%` (seed `7`), while seed `8675309` is about `34.8%` desert. The regression requires the maximum to remain below `70%`.
- Focused climate and biome test executables passed.
- Full `ctest --test-dir out/build/debug-with-tests --output-on-failure` passed, 16/16.
- `scripts/build.ps1 -Preset debug-no-tests` and `scripts/build.ps1 -Preset release-no-tests` passed.
- `git diff --check` passed; Git reported only the repository's LF-to-CRLF conversion warnings.

The task was reopened after the initial verification, and all verification was rerun for the regional-variation scope.

- Added cross-seed coastal-orientation coverage on a symmetric synthetic realm. Across seeds `1..32`, dominant wet coasts currently distribute `W/E/N/S = 7/10/6/9`; every side must win at least four times and none may win more than twelve.
- Added coverage proving regional variation is deterministic, exactly neutralizable, materially changes more than half a symmetric test map, and remains smoother between adjacent cells than between distant regions.
- Focused climate and biome tests passed after the regional extension.
- Full `ctest --test-dir out/build/debug-with-tests --output-on-failure` passed, 16/16, after reopening.
- `scripts/build.ps1 -Preset debug-no-tests` and `scripts/build.ps1 -Preset release-no-tests` passed after reopening.
- Final `git diff --check` passed with only the repository's LF-to-CRLF conversion warnings.

## Commit Message

`fix(procgen): add seed-varied regional circulation`
