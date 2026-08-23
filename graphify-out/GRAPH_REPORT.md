# Graph Report - SDL_Trading  (2026-08-22)

## Corpus Check
- Corpus is ~15,688 words - fits in a single context window. You may not need a graph.

## Summary
- 332 nodes · 646 edges · 13 communities
- Extraction: 83% EXTRACTED · 17% INFERRED · 0% AMBIGUOUS · INFERRED: 112 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

## Community Hubs (Navigation)
- App State & Core Models
- Alerts Engine
- UI Theming
- Alert Modal Events
- UI Drawing & Layout
- Core Data Types & Includes
- App Layout Dimensions
- Favorites State Management
- Network Fetching
- Order Book Model
- Chart Drawings
- Candlestick Data Model
- Trade Data Model

## God Nodes (most connected - your core abstractions)
1. `AppState` - 134 edges
2. `ThemeColors` - 37 edges
3. `SDL_AppEvent()` - 27 edges
4. `AppLayout` - 22 edges
5. `draw_table()` - 19 edges
6. `Alert` - 19 edges
7. `PairInfo` - 17 edges
8. `point_in()` - 17 edges
9. `ui_fill_rect()` - 16 edges
10. `ui_draw_text()` - 15 edges

## Surprising Connections (you probably didn't know these)
- `Architecture Overview` --references--> `SDL_AppEvent()`  [EXTRACTED]
  AGENTS.md → src/main.cpp
- `Architecture Overview` --references--> `SDL_AppIterate()`  [EXTRACTED]
  AGENTS.md → src/main.cpp
- `Architecture Overview` --references--> `SDL_AppInit()`  [EXTRACTED]
  AGENTS.md → src/main.cpp
- `SDL_AppEvent()` --calls--> `alerts_new_id()`  [INFERRED]
  src/main.cpp → src/alerts.cpp
- `SDL_AppEvent()` --calls--> `alerts_save()`  [INFERRED]
  src/main.cpp → src/alerts.cpp

## Import Cycles
- None detected.

## Hyperedges (group relationships)
- **SDL3 Callback Paradigm** — src_main_sdl_appinit, src_main_sdl_appevent, src_main_sdl_appiterate [EXTRACTED 1.00]

## Communities (13 total, 0 thin omitted)

### Community 0 - "App State & Core Models"
Cohesion: 0.02
Nodes (86): MarketTab, RightTab, SDL_Cursor, SDL_Window, AppState, active_tab, active_tool, alert_modal_draft (+78 more)

### Community 1 - "Alerts Engine"
Cohesion: 0.06
Nodes (39): alert_type_label(), alerts_engine(), alerts_load(), alerts_new_id(), alerts_save(), AlertType, string, vector (+31 more)

### Community 2 - "UI Theming"
Cohesion: 0.06
Nodes (38): Theme, dark_theme(), get_theme(), SDL_Color, light_theme(), theme_label(), theme_next(), ThemeColors (+30 more)

### Community 3 - "Alert Modal Events"
Cohesion: 0.11
Nodes (36): SDL_Event, alert_freq_label(), AlertFrequency, get_modal_layout(), handle_alert_modal_click(), hit_alert_del_btn(), hit_alert_row(), hit_alert_toggle_btn() (+28 more)

### Community 4 - "UI Drawing & Layout"
Cohesion: 0.25
Nodes (30): draw_alert_modal(), draw_alerts(), draw_chart(), draw_dividers(), draw_notification(), draw_toolbox(), SDL_AppIterate(), draw_table() (+22 more)

### Community 5 - "Core Data Types & Includes"
Cohesion: 0.22
Nodes (6): atomic, mutex, string, vector, thread, unordered_set

### Community 6 - "App Layout Dimensions"
Cohesion: 0.10
Nodes (21): AppLayout, chart_h, chart_w, chart_x, chart_y, right_tabs_h, right_tabs_w, right_tabs_x (+13 more)

### Community 7 - "Favorites State Management"
Cohesion: 0.23
Nodes (13): Architecture Overview, SDL_AppResult, string, fav_has(), fav_load(), fav_save(), fav_toggle(), FavoritesState (+5 more)

### Community 8 - "Network Fetching"
Cohesion: 0.27
Nodes (8): fetch_candles(), fetch_pairs(), string, http_get(), http_post(), telegram_send(), fetch_orderbook(), fetch_trades()

### Community 9 - "Order Book Model"
Cohesion: 0.22
Nodes (9): mutex, vector, OBEntry, price, qty, OrderBook, asks, bids (+1 more)

### Community 10 - "Chart Drawings"
Cohesion: 0.25
Nodes (8): ChartDrawing, p1, p2, t1, t2, text, tool, Tool

### Community 11 - "Candlestick Data Model"
Cohesion: 0.29
Nodes (7): Candle, close, high, low, open, open_time, volume

### Community 12 - "Trade Data Model"
Cohesion: 0.33
Nodes (6): Trade, buyer_maker, id, price, qty, time_ms

## Knowledge Gaps
- **176 isolated node(s):** `win_w`, `win_h`, `TOOLBOX_W`, `split_ratio`, `vsplit_ratio` (+171 more)
  These have ≤1 connection - possible missing edges or undocumented components.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `AppState` connect `App State & Core Models` to `Alerts Engine`, `UI Theming`, `Alert Modal Events`, `UI Drawing & Layout`, `Core Data Types & Includes`, `App Layout Dimensions`, `Favorites State Management`, `Network Fetching`, `Order Book Model`, `Chart Drawings`, `Candlestick Data Model`, `Trade Data Model`?**
  _High betweenness centrality (0.771) - this node is a cross-community bridge._
- **Why does `ThemeColors` connect `UI Theming` to `App State & Core Models`, `UI Drawing & Layout`, `Core Data Types & Includes`?**
  _High betweenness centrality (0.196) - this node is a cross-community bridge._
- **Why does `AppLayout` connect `App Layout Dimensions` to `App State & Core Models`, `Core Data Types & Includes`?**
  _High betweenness centrality (0.117) - this node is a cross-community bridge._
- **Are the 22 inferred relationships involving `SDL_AppEvent()` (e.g. with `alerts_new_id()` and `alerts_save()`) actually correct?**
  _`SDL_AppEvent()` has 22 INFERRED edges - model-reasoned connections that need verification._
- **Are the 12 inferred relationships involving `draw_table()` (e.g. with `SDL_AppIterate()` and `fav_has()`) actually correct?**
  _`draw_table()` has 12 INFERRED edges - model-reasoned connections that need verification._
- **What connects `win_w`, `win_h`, `TOOLBOX_W` to the rest of the system?**
  _176 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `App State & Core Models` be split into smaller, more focused modules?**
  _Cohesion score 0.023255813953488372 - nodes in this community are weakly interconnected._