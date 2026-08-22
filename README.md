# SDL Trading Platform

A high-performance, multithreaded cryptocurrency trading interface built from scratch in C++ using SDL3.

## Overview

This project is a custom-built trading terminal designed for speed and responsiveness. It provides real-time market data visualization, order book tracking, and chart drawing capabilities without the overhead of heavy web frameworks like Electron. Built directly on top of SDL3, it utilizes a highly optimized render loop and a multi-threaded architecture to ensure the UI remains smooth (60+ FPS) while simultaneously polling external APIs for market data.

## Features

- **Interactive Candlestick Charts**: Smooth panning, price/time zooming, and auto-scaling.
- **Drawing Toolbox**: A left-hand toolbox to annotate charts with interactive vector tools:
  - Cursor & Crosshair
  - Trend Lines
  - Horizontal Lines
  - Text Notes
- **Real-time Market Panels**:
  - **Order Book**: Live bid/ask spread and depth visualization.
  - **Recent Trades**: Scrolling list of the most recent market executions.
  - **Market Table**: Browse Spot and Futures pairs, sort by volume or 24h change, and save symbols to Favorites.
- **Alerts System**: Built-in modal to create, edit, delete, and toggle custom price alerts. Saves locally to disk.
- **Customizable Layout**: Fully resizable panels with draggable horizontal and vertical dividers.
- **Custom UI Engine**: A lightweight, immediate-mode style UI rendering system with light/dark theme support.

## Architecture

The application is structured around the modern SDL3 callback architecture (`SDL_AppInit`, `SDL_AppIterate`, `SDL_AppEvent`):
- **State Management**: A centralized `AppState` struct (`src/app_state.hpp`) holds the entire application state, making it trivial to pass context across modules.
- **Concurrency**: Network requests and API polling are handled in background threads (`candle_thread`, `ob_thread`, `trades_thread`, etc.). Data is synced to the main render thread via standard `std::mutex` locks.
- **Rendering**: UI components are drawn frame-by-frame in `SDL_AppIterate` using custom wrappers defined in `src/ui_utils.hpp`.

## Requirements

- A modern C++ compiler (C++17 or higher recommended)
- **CMake** (3.15+)
- **SDL3** and **SDL3_ttf** libraries

## Build Instructions

The project uses CMake for out-of-source builds.

```bash
# 1. Create a build directory and configure the project
cmake -B build -S .

# 2. Compile the project (using 4 parallel jobs)
cmake --build build -j4

# 3. Run the application
./build/sdl-trading
```

## Project Structure

- `src/main.cpp`: SDL3 lifecycle hooks, global event routing, and layout calculations.
- `src/app_state.hpp`: Core definitions for `AppState`, layouts, and shared variables.
- `src/chart.cpp`: Candlestick rendering, grid logic, and user drawing tools.
- `src/market_table.cpp`: Top-right symbol table, search, and sorting logic.
- `src/orderbook.cpp` & `src/trades.cpp`: Binance API polling and right-panel rendering.
- `src/alerts.cpp`: Alert management and modal editor rendering.
- `src/ui_utils.cpp`: Text and geometry rendering wrappers.
- `src/theme.cpp`: Color palettes and theme toggling.
