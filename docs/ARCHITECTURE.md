# SDL Trading — Architecture

This document describes the modular layout of the codebase and the purpose of each public function.

## Directory Structure

```
src/
├── main.cpp              # SDL3 callback entry point (thin wrapper)
├── core/                 # Shared types, state, layout
├── app/                  # SDL3 lifecycle (init, event, render, quit)
├── window/               # Borderless window chrome & snap
├── ui/                   # Reusable drawing primitives & chrome widgets
├── services/             # Network, persistence, background helpers
└── modules/              # Feature panels (chart, table, alerts, …)
```

## Data Flow

```
SDL3 callbacks (main.cpp)
    └── app/          init · event · render · quit
            └── core/app_state.hpp   (single shared AppState*)
                    ├── window/      titlebar, snap, hit-test
                    ├── ui/          draw helpers, tabs, toolbox
                    ├── modules/     chart, table, orderbook, …
                    └── services/    HTTP, favorites, account mock
```

All UI code reads layout coordinates from `state->layout`, updated each frame by `update_layout()`.

---

## `src/main.cpp`

| Function | Description |
|----------|-------------|
| `SDL_AppInit` | Forwards to `app_init()` |
| `SDL_AppEvent` | Forwards to `app_event()` |
| `SDL_AppIterate` | Forwards to `app_iterate()` |
| `SDL_AppQuit` | Forwards to `app_quit()` |

---

## `core/` — Shared foundations

### `types.hpp`
Shared enums and structs: `PairInfo`, `Candle`, `Alert`, `MarketTab`, `RightTab`, `AppTab`, `ResizeDir`, `SnapZone`, etc.

### `constants.hpp`
Window defaults, font paths, icon UTF-8 glyphs, JSON file paths.

### `app_state.hpp`
Central `AppState` struct holding SDL handles, chart state, market data, mutexes, and thread handles. Passed as `AppState*` to every module.

### `layout.hpp` / `layout.cpp`

| Function | Description |
|----------|-------------|
| `update_layout` | Recomputes all panel rectangles (`chart_*`, `table_*`, `toolbox_*`, etc.) from the current window size and split ratios |

---

## `app/` — Application lifecycle

### `app_init.hpp` / `app_init.cpp`

| Function | Description |
|----------|-------------|
| `app_init` | Initializes SDL/TTF/cURL, creates borderless window + renderer, loads fonts, restores favorites/alerts, starts all background threads |

### `app_event.hpp` / `app_event.cpp`

| Function | Description |
|----------|-------------|
| `app_event` | Routes all input: keyboard, text input, mouse wheel, mouse down/up/motion, window resize/move. Dispatches to titlebar, tabs, chart, table, alerts modal, and panel dividers |

### `app_render.hpp` / `app_render.cpp`

| Function | Description |
|----------|-------------|
| `app_iterate` | Per-frame render loop: updates snap/layout, clears screen, draws chrome + active tab content, notification, alert modal, snap overlay |

### `app_quit.hpp` / `app_quit.cpp`

| Function | Description |
|----------|-------------|
| `app_quit` | Sets `running = false`, joins threads, saves favorites/alerts, destroys fonts/cursors/window, quits SDL |

---

## `window/` — Borderless window management

### `titlebar.hpp` / `titlebar.cpp`

| Function | Description |
|----------|-------------|
| `hit_close_btn` | Hit-test for the ✕ close button |
| `hit_max_btn` | Hit-test for the □ maximize/restore button |
| `hit_min_btn` | Hit-test for the — minimize button |
| `draw_titlebar` | Renders live stats (price, alerts, PNL) and window control buttons |

### `window_snap.hpp` / `window_snap.cpp`

| Function | Description |
|----------|-------------|
| `get_display_bounds` | Returns usable monitor bounds for the display under the mouse |
| `compute_snap_zone` | Detects TOP/LEFT/RIGHT snap zone from global mouse position |
| `snap_target_rect` | Returns target window rectangle for a snap zone |
| `save_window_restore_rect` | Stores current geometry in `win_restore_*` |
| `maximize_window_to_display` | Fills the usable display area |
| `restore_window` | Restores pre-maximize geometry |
| `begin_snap_drag` | Starts snap tracking on titlebar drag; restores if maximized |
| `apply_snap` | Resizes/repositions window to match snap zone |
| `update_snap_during_drag` | Polls mouse during native drag; applies snap on release |
| `draw_snap_overlay` | Draws blue preview overlay during snap drag |

### `window_hit_test.hpp` / `window_hit_test.cpp`

| Function | Description |
|----------|-------------|
| `get_resize_dir` | Returns border resize direction for a window-local point |
| `cursor_for_resize` | Maps `ResizeDir` to the appropriate system cursor |
| `hit_test_callback` | SDL hit-test callback: draggable titlebar, resizable borders |

---

## `ui/` — Drawing & chrome widgets

### `ui_utils.hpp` / `ui_utils.cpp`

| Function | Description |
|----------|-------------|
| `ui_fill_rect` | Filled axis-aligned rectangle |
| `ui_fill_rect_rounded` | Filled rectangle with rounded corners |
| `ui_draw_rect` | Rectangle outline |
| `ui_draw_line` | Line segment |
| `ui_draw_text` | Left-aligned text |
| `ui_draw_text_centered` | Centered text in a box |
| `ui_draw_text_right` | Right-aligned text |
| `ui_text_width` / `ui_text_height` | Text measurement |
| `point_in` | Axis-aligned hit-test |
| `fmt_price` / `fmt_pct` / `fmt_vol` / `fmt_time` | Number/time formatters |
| `ui_button` | Draws + hit-tests a labeled button |
| `ui_search_bar` | Draws + hit-tests a search input bar |

### `theme.hpp` / `theme.cpp`

| Function | Description |
|----------|-------------|
| `dark_theme` / `light_theme` | Returns a complete `ThemeColors` palette |
| `get_theme` | Resolves `Theme::SYSTEM` to OS preference, else dark/light |
| `theme_label` | Returns display label for theme toggle button |
| `theme_next` | Cycles Dark → Light → System |

### `app_tabs.hpp` / `app_tabs.cpp`

| Function | Description |
|----------|-------------|
| `draw_app_tabs` | Renders MARKETS / TRADES / BOTS / … navigation bar |
| `hit_app_tab` | Returns tab index under mouse, or -1 |

### `toolbox.hpp` / `toolbox.cpp`

| Function | Description |
|----------|-------------|
| `draw_toolbox` | Renders chart drawing tools strip with tooltips |
| `hit_toolbox_tool` | Hit-test for a tool button; returns selected tool |
| `hit_toolbox_clear` | Hit-test for the Clear All drawings button |

### `chrome.hpp` / `chrome.cpp`

| Function | Description |
|----------|-------------|
| `draw_dividers` | Renders chart\|panel and table\|orderbook resize dividers |
| `draw_notification` | Auto-fading alert notification banner (4 s) |
| `draw_placeholder` | Centered "Coming Soon" text for unimplemented tabs |

---

## `services/` — Infrastructure

### `network.hpp` / `network.cpp`

| Function | Description |
|----------|-------------|
| `http_get` | Synchronous HTTP GET via libcurl |
| `http_post` | Synchronous HTTP POST |
| `telegram_send` | Sends a Telegram bot message (if `ENABLE_TELEGRAM`) |

### `favorites.hpp` / `favorites.cpp`

| Function | Description |
|----------|-------------|
| `fav_has` | Returns whether a symbol is favorited |
| `fav_toggle` | Adds/removes a symbol from favorites |
| `fav_save` / `fav_load` | JSON persistence to `favorites.json` |

### `account.hpp` / `account.cpp`

| Function | Description |
|----------|-------------|
| `mock_account_thread` | Background thread updating mock balance/PNL for the titlebar |

---

## `modules/` — Feature panels

### `chart.hpp` / `chart.cpp`

| Function | Description |
|----------|-------------|
| `fetch_candles` | Background thread: polls Binance klines for `chart_symbol` |
| `draw_chart` | Renders candlesticks, price axis, timeframe buttons, crosshair, drawings |

### `market_table.hpp` / `market_table.cpp`

| Function | Description |
|----------|-------------|
| `fetch_pairs` | Background thread: polls Binance 24h tickers (SPOT + FUTURES) |
| `draw_table` | Renders market table with tabs, search, sort headers, rows |
| `get_sorted_filtered_pairs` | Returns sorted/filtered pair list for active tab |
| `hit_tab` | Hit-test SPOT/FUTURES/FAV tab buttons |
| `hit_col_header` | Hit-test sortable column headers |
| `hit_table_row` | Returns visible row index under mouse |
| `hit_search_bar` | Hit-test search input area |
| `hit_fav_star` | Hit-test favourite star on a row |
| `hit_theme_button` | Hit-test dark/light theme toggle |

### `orderbook.hpp` / `orderbook.cpp`

| Function | Description |
|----------|-------------|
| `fetch_orderbook` | Background thread: polls Binance depth for `chart_symbol` |
| `draw_orderbook` | Renders bid/ask ladder with depth bars |

### `trades.hpp` / `trades.cpp`

| Function | Description |
|----------|-------------|
| `fetch_trades` | Background thread: polls Binance recent trades |
| `draw_trades` | Renders recent trades list |

### `alerts.hpp` / `alerts.cpp`

| Function | Description |
|----------|-------------|
| `alerts_engine` | Background thread: evaluates alert conditions every 5 s |
| `draw_alerts` | Renders alerts list in the right panel |
| `draw_alert_modal` | Renders create/edit alert overlay modal |
| `handle_alert_modal_click` | Processes modal field/button clicks |
| `update_alert_modal_cursor` | Sets cursor shape over modal interactive elements |
| `handle_alert_modal_text` | Appends typed text to focused modal field |
| `handle_alert_modal_backspace` | Deletes char in focused modal field |
| `alert_commit_threshold` | Parses threshold string buffer into draft alert |
| `alert_type_label` / `alert_freq_label` | Human-readable enum labels |
| `alerts_save` / `alerts_load` | JSON persistence to `alerts.json` |
| `alerts_new_id` | Generates a unique alert ID |
| `hit_alerts_add_button` | Hit-test "+ Add Alert" button |
| `hit_alert_row` | Returns alert row index under mouse |
| `hit_alert_del_btn` / `hit_alert_toggle_btn` | Hit-test row action buttons |

### `right_panel.hpp` / `right_panel.cpp`

| Function | Description |
|----------|-------------|
| `draw_right_panel` | Renders ORDER BOOK \| TRADES \| ALERTS tab bar + active content |
| `hit_right_tab` | Returns right-panel tab index under mouse |
| `right_panel_content_rect` | Returns content area bounds below the tab bar |

---

## Adding New Features

| Task | Where to edit |
|------|---------------|
| New drawing tool | `ui/toolbox.*`, `app/app_event.cpp` (chart clicks), `modules/chart.*` |
| New app tab | `ui/app_tabs.*`, `app/app_render.cpp`, `app/app_event.cpp` |
| New right-panel tab | `core/types.hpp`, `modules/right_panel.*`, `app/app_event.cpp` |
| New alert type | `core/types.hpp`, `modules/alerts.*` |
| Window snap behavior | `window/window_snap.*` |
| Panel layout | `core/layout.*`, `core/app_state.hpp` (`AppLayout`) |

## Build

```bash
cmake -B build -S .
cmake --build build -j4
./build/sdl-trading
```
