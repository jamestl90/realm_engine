# UI Feature Inventory

This document tracks the UI system features currently present in the engine. It is a capability snapshot, not a detailed architecture spec.

## Core System

- `UIElement`: base element with IDs, names, parent/child hierarchy, bounds, size constraints, visibility, enable/disable state, layout invalidation, update, and hit testing.
- `UIManager`: owns the root UI tree, performs layout/update, dispatches SDL input events, tracks hover/capture state, manages keyboard focus, starts/stops SDL text input for text controls, and maps window coordinates into logical UI coordinates.
- `UIRenderer`: renders the UI tree after world sprites using the SDL3 GPU sprite pipeline.

## Layout

- `LayoutContainer`: base container with padding and optional background colour.
- `StackPanel`: horizontal or vertical child layout with spacing.
- `GridPanel`: row/column layout with definitions, child row/column placement, and row/column spans.
- Common layout data: `Rect`, `SizeConstraints`, `Thickness`, `HorizontalAlignment`, and `VerticalAlignment`.

## Primitives

- `TextBlock`: static text with colour, font size, alignment, and font family field.
- `Image`: texture-backed image with optional region name, tint, source size, and stretch modes (`None`, `Fill`, `Uniform`, `UniformToFill`).
- `Rectangle`: solid fill and border rendering.
- `Colour`: simple RGBA colour value with white, black, and transparent helpers.

## Controls

- `Button`: clickable text button with normal, hover, pressed, focused, and disabled visual state support. Supports click callback, colours, border thickness, padding, and font size.
- `RepeatButton`: press-and-hold button that activates immediately, waits for a configurable initial delay, then repeats at a configurable interval. Its callback can stop repetition when a control reaches a value bound.
- `Slider`: horizontal range control with min/max/step snapping, value-change callback, click-to-set and drag interaction, disabled-state rendering, and pointer-cancellation handling.
- `TextBox`: single-line text input with placeholder text, cursor movement, selection, max length, read-only mode, password masking, submit callback, text-changed callback, focus border colour, selection colour, and blinking cursor.
- `ComboBox`: dropdown selection control with item list, selected index/item, placeholder text, open/close state, hovered item tracking, selection callback, and configurable colours/border/font/padding.

## Input And Focus

- `InputSurface`: mouse, wheel, keyboard, text input, hover, and pressed-state callbacks.
- Active pointer interactions are cancelled when the pointer leaves the window or the window loses focus.
- Pressed controls receive pointer motion while the press remains active, allowing drag controls such as sliders to continue updating outside their own hover bounds.
- `FocusableControl`: keyboard focus support, tab index, focus/blur callbacks, and activation via keyboard.
- `FocusManager`: tracks focused control and supports focus navigation.
- Text input is routed to the focused control when it reports `wantsTextInput()`.

## Styling And Animation

- `Style`: state-aware property map for colours, dimensions, alignment, padding, margin, and font family.
- `StyledControl`: base class for controls with style and visual state tracking.
- `DefaultStyles`: factory hooks for button, text box, and panel styles.
- `AnimationManager`: owns and updates active animations.
- Animations currently include float property animation, colour animation, looping, delay, auto-reverse, pause/resume, and easing helpers.

## Rendering Capabilities

- UI rendering batches rectangle, text, and textured-rectangle commands by texture.
- Text rendering uses SDL_ttf's GPU text engine and reusable glyph atlases; glyph quads are tinted and submitted through the UI sprite pipeline.
- Changing dynamic text does not allocate a standalone GPU texture for each complete string. Previously unseen glyphs may be added to an atlas page as needed.
- Font faces are loaded once per requested point size and reused through the font cache.
- `TextBlock`, `Button`, `TextBox`, and `ComboBox` layout uses SDL_ttf metrics for the same font size used during rendering.
- Button labels, text alignment, text-box selections/cursors, and combo-box text placement use measured glyph dimensions rather than character-count estimates.
- Solid colour UI uses the texture manager's generated 1x1 white texture.
- Borders are rendered as rectangular edge quads.
- The UI pass renders over the sprite/world pass and preserves existing swapchain contents.

## Current Notes

- Corner radius values exist on some controls/primitives, but current rendering uses rectangular quads.
- `TextBlock::fontFamily` is stored, but per-element font-face selection is not yet connected; UI text currently uses size variants of the renderer's default font face.
- The combo box dropdown is rendered internally by `UIRenderer` in a final popup pass, above normal UI tree content and without contributing to parent layout size.
- The current sample UI in `RogueFarmGame` demonstrates a button, text box, and combo box wired to window resizing.
