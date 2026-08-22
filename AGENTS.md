# Agent Instructions (AGENTS.md)

This file contains guidelines and context for LLM agents (like Claude, Cursor, Cline, or Antigravity) working on the `SDL_Trading` codebase. Read this carefully before modifying the architecture or implementing new features.

## Architecture Overview

1. **SDL3 Callback Paradigm**: 
   The application uses the new SDL3 main callbacks.
   - `SDL_AppInit` (setup, threads, fonts)
   - `SDL_AppEvent` (input routing)
   - `SDL_AppIterate` (render loop)
   These are located in `src/main.cpp`. Avoid introducing traditional blocking `while(1)` loops.

2. **State Management**:
   The entire application state is stored in the `AppState` struct defined in `src/app_state.hpp`. Pass the `AppState*` pointer to all major update and render functions.

3. **Rendering Paradigm**:
   We use a custom, lightweight immediate-mode-style drawing wrapper over SDL3 rendering (see `src/ui_utils.hpp`).
   - DO NOT introduce heavy UI frameworks (like ImGui or Qt) unless explicitly requested by the user. 
   - Use `ui_fill_rect`, `ui_draw_rect`, `ui_draw_line`, `ui_draw_text` for rendering.
   - Respect layout boundaries. Calculate coordinates relative to the panel configurations in `AppLayout` (e.g., `state->layout.chart_x`).

4. **Multi-threading & Mutexes**:
   The UI must never block. Network requests (like Binance API calls) run in background threads (`candle_thread`, `ob_thread`, etc.).
   - Shared data (e.g., `state->candles`, `state->trades`, `state->orderbook`) **MUST** be locked using `std::lock_guard<std::mutex> lock(state-><target>_mtx);` before reading or writing.
   - Ensure mutexes are never nested in a way that causes deadlocks. Keep locks as short-lived as possible (e.g., lock, copy data, unlock).

## Event Handling (`SDL_AppEvent`)

All mouse and keyboard events are handled in `src/main.cpp`.
- Event routing is strictly coordinate-based. 
- When adding a new interactive element, ensure its click handler is placed in the correct spatial block in `SDL_AppEvent` (e.g., "Chart area clicks", "Right panel area clicks").
- Return `SDL_APP_CONTINUE` immediately after handling a click to prevent click-through.

## Build Instructions

To build the project for testing your changes:
```bash
cmake -B build -S .
cmake --build build -j4
```
Execution:
```bash
./build/sdl-trading
```

## Adding New Features

1. **Adding a new Tool to the Toolbox**:
   - Add the tool to `AppState::Tool` enum.
   - Update `draw_toolbox` in `main.cpp` with a new icon/tooltip.
   - Handle the drawing interaction inside the chart area in `main.cpp` and `chart.cpp` (`drawings` array).
2. **Adding a new Tab**:
   - Add to `RightTab` or `MarketTab` enum in `types.hpp`.
   - Update `hit_right_tab` or `hit_tab` hit-testing functions.
   - Add a dispatch call in `draw_right_panel` or `draw_table`.

## Style & Conventions

- Use standard C++17/20 features.
- Avoid excessive OOP (classes/inheritance) where simple C-style structs and functions suffice.
- Use `snake_case` for variables and functions.
- Use `PascalCase` for Structs and Enums.
- Use 2 spaces for indentation.
- Group logical blocks visually with ASCII dividers (e.g., `// ── Section ──`).


## Modifying UI

- UI layout coordinates are strictly defined in `AppLayout`. Avoid magic numbers for global layout coordinates; compute them relative to `state->layout`.
- Update click hit-boxes in `SDL_AppEvent` (`main.cpp`) whenever changing the visual position of clickable elements.
