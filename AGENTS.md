# Agent Instructions (AGENTS.md)

Guidelines for LLM agents working on the `SDL_Trading` codebase.

## Architecture Overview

See **[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)** for the full module map and function reference.

```
src/
├── main.cpp          ← SDL3 callback entry (thin wrapper)
├── core/             ← types, constants, AppState, layout
├── app/              ← SDL_AppInit / AppEvent / AppIterate / AppQuit
├── window/           ← titlebar, snap, hit-test (borderless window)
├── ui/               ← draw primitives, tabs, toolbox, chrome
├── services/         ← HTTP, favorites, account mock
└── modules/          ← chart, market_table, orderbook, trades, alerts, right_panel
```

### SDL3 Callback Paradigm

The application uses SDL3 main callbacks, forwarded from `main.cpp` to `app/`:

- `app_init` — setup, threads, fonts
- `app_event` — input routing
- `app_iterate` — render loop
- `app_quit` — shutdown

Avoid introducing traditional blocking `while(1)` loops.

### State Management

All application state lives in `AppState` (`core/app_state.hpp`). Pass `AppState*` to every update/render function.

### Rendering Paradigm

Lightweight immediate-mode drawing via `ui/ui_utils.hpp`:

- `ui_fill_rect`, `ui_draw_rect`, `ui_draw_line`, `ui_draw_text`
- DO NOT introduce heavy UI frameworks unless explicitly requested
- Compute coordinates relative to `state->layout` (`core/layout.hpp`)

### Multi-threading & Mutexes

Network requests run in background threads. Lock shared data before access:

```cpp
std::lock_guard<std::mutex> lock(state->candles_mtx);
```

Keep locks short-lived; never nest mutexes in ways that cause deadlocks.

## Event Handling

Input routing lives in `app/app_event.cpp`. It is strictly coordinate-based:

- Titlebar/window → `window/titlebar.*`, `window/window_snap.*`
- App tabs → `ui/app_tabs.*`
- Chart/toolbox → `ui/toolbox.*`, `modules/chart.*`
- Market table → `modules/market_table.*`
- Right panel → `modules/right_panel.*`, `modules/alerts.*`

Return `SDL_APP_CONTINUE` immediately after handling a click to prevent click-through.

## Build Instructions

```bash
cmake -B build -S .
cmake --build build -j4
./build/sdl-trading
```

## Adding New Features

1. **New toolbox tool**: add to `AppState::Tool`, update `ui/toolbox.*`, handle in `app/app_event.cpp` + `modules/chart.*`
2. **New tab**: add enum in `core/types.hpp`, update hit-test in the relevant module, dispatch in `app/app_render.cpp`
3. **New panel layout field**: extend `AppLayout` in `core/app_state.hpp`, update `core/layout.cpp` and hit-tests in `app/app_event.cpp`

## Style & Conventions

- C++17/20, snake_case functions, PascalCase structs/enums
- 2-space indentation
- ASCII section dividers (`// ── Section ──`)
- Avoid excessive OOP; prefer structs + free functions
- Document public functions with `///` comments in headers
