# graphics-engine

Guard rails for working on the engine code. Build and gates live in [README.md](README.md), open points as issues.

## Principles

The engine follows the conventions of its first consumer, written once in [decade's AGENTS.md](https://github.com/schneeregenflocke/decade/blob/main/AGENTS.md): refactoring, naming, ownership, components, and the rule that warnings break the build and never get suppressed. Language and documentation rules live in the superproject.

## Architecture

- `src/graphics/` draws a scene graph of shapes and FreeType text through libepoxy into whichever context the caller made current. It knows no window and no toolkit; the consumer owns the context.
- `src/physics/` mirrors the pickable boxes into a Bullet collision world and answers a ray through the cursor with a `PickId`.
- `shaders/` holds the GLSL; [embedded_shaders.hpp](src/graphics/embedded_shaders.hpp) carries it into the library with `#embed`.

## Decisions

### Why add_subdirectory and not an installed package

The consumer builds the engine in its own tree with its own flags, so a sanitizer build instruments both alike. Tests and gate targets exist only when the engine is the top-level project; inside decade their names would collide with decade's own.

### Why a log switch of its own

The engine reads no command line. `graphics_log::SetLogEnabled` stays off until the application turns it on, as decade does from `--debug-log`.
