# SDL Trading Platform

A high-performance, multithreaded cryptocurrency trading interface built from scratch in C++ using SDL3.

## Overview

This project is a custom-built trading terminal designed for speed and responsiveness. It provides real-time market data visualization, order book tracking, and chart drawing capabilities without the overhead of heavy web frameworks like Electron. Built directly on top of SDL3, it uses a callback-driven render loop and a multi-threaded architecture so the UI stays smooth (60+ FPS) while background threads poll external APIs for market data.

## Features

- **Interactive Candlestick Charts**: Smooth panning, price/time zooming, auto-scaling, and multiple timeframes (`1m` … `1w`).
- **Drawing Toolbox**: Left-hand toolbox to annotate charts with interactive vector tools:
  - Cursor & Crosshair
  - Trend Lines
  - Horizontal Lines
  - Text Notes
- **App Navigation Tabs**: Top-level tabs (Markets, Trades, Bots, Strategies, Backtesting, Exchanges, Account, Configuration). Unimplemented tabs show a placeholder.
- **Real-time Market Panels**:
  - **Order Book**: Live bid/ask spread and depth visualization.
  - **Recent Trades**: Scrolling list of the most recent market executions.
  - **Market Table**: Browse Spot and Futures pairs, sort by volume or 24h change, search, and save symbols to Favorites.
- **Alerts System**: Create, edit, delete, and toggle custom price alerts. Persists to `alerts.json`. Optional Telegram notifications via libcurl.
- **Borderless Window Chrome**: Custom titlebar with live stats, minimize/maximize/restore/close, edge resize, and Aero-style snap (top / left / right).
- **Customizable Layout**: Resizable panels with draggable horizontal and vertical dividers.
- **Custom UI Engine**: Lightweight immediate-mode drawing with Dark / Light / System theme support.

## Architecture

The application uses the SDL3 main-callback paradigm (`SDL_AppInit`, `SDL_AppIterate`, `SDL_AppEvent`, `SDL_AppQuit`), forwarded from a thin `src/main.cpp` into `src/app/`:

| Concern | Location |
|---------|----------|
| Shared types, `AppState`, layout | `src/core/` |
| Lifecycle (init / event / render / quit) | `src/app/` |
| Borderless window, titlebar, snap, hit-test | `src/window/` |
| Draw primitives, tabs, toolbox, chrome | `src/ui/` |
| HTTP, favorites, account mock | `src/services/` |
| Chart, table, orderbook, trades, alerts | `src/modules/` |

- **State**: A centralized `AppState` (`src/core/app_state.hpp`) is passed as `AppState*` to every update/render function. Layout rectangles live in `state->layout`, recomputed by `update_layout()`.
- **Concurrency**: Background threads (`fetch_candles`, `fetch_orderbook`, `fetch_trades`, `fetch_pairs`, `alerts_engine`, …) poll APIs; shared data is guarded with `std::mutex`.
- **Rendering**: Immediate-mode helpers in `src/ui/ui_utils.hpp` (`ui_fill_rect`, `ui_draw_text`, …).

For the full module map and public function reference, see **[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)**.

## Requirements

- C++20 compiler
- **CMake** 3.16+
- **libcurl**
- By default, CMake fetches via FetchContent (`DOWNLOAD_DEPENDENCIES=ON`):
  - **SDL3**, **SDL3_ttf**, **SDL3_image**
  - **nlohmann/json**
- Or install those packages system-wide and configure with `-DDOWNLOAD_DEPENDENCIES=OFF`
- Optional: `-DENABLE_TELEGRAM=ON` (default) for Telegram alert notifications

## Build Instructions

```bash
# Configure
cmake -B build -S .

# Build (example: 4 parallel jobs)
cmake --build build -j4

# Run
./build/sdl-trading
```

## Project Structure

```
SDL_Trading/
├── CMakeLists.txt
├── README.md
├── AGENTS.md                 # Guidelines for LLM agents
├── docs/
│   └── ARCHITECTURE.md       # Module map & function reference
├── res/fonts/                # JetBrains Mono + icon font (sdl-trading.ttf)
├── alerts.json / favorites.json   # Local persistence (runtime)
└── src/
    ├── main.cpp              # SDL3 callback entry (thin wrapper)
    ├── core/                 # Shared foundations
    │   ├── types.hpp         # Candle, PairInfo, Alert, AppTab, …
    │   ├── constants.hpp     # Window defaults, font/icon paths
    │   ├── app_state.hpp     # Central AppState + AppLayout
    │   ├── layout.hpp
    │   └── layout.cpp        # update_layout()
    ├── app/                  # SDL3 lifecycle
    │   ├── app_init.*        # Setup, fonts, threads
    │   ├── app_event.*       # Input routing (coordinate-based)
    │   ├── app_render.*      # Per-frame draw loop
    │   └── app_quit.*        # Shutdown & persistence
    ├── window/               # Borderless window chrome
    │   ├── titlebar.*
    │   ├── window_snap.*
    │   └── window_hit_test.*
    ├── ui/                   # Drawing & chrome widgets
    │   ├── ui_utils.*
    │   ├── theme.*
    │   ├── app_tabs.*
    │   ├── toolbox.*
    │   └── chrome.*          # Dividers, notifications, placeholders
    ├── services/             # Infrastructure
    │   ├── network.*         # HTTP GET/POST, Telegram
    │   ├── favorites.*
    │   └── account.*         # Mock balance / PNL thread
    └── modules/              # Feature panels
        ├── chart.*
        ├── market_table.*
        ├── orderbook.*
        ├── trades.*
        ├── alerts.*
        └── right_panel.*
```

## License

See [LICENSE](LICENSE).
