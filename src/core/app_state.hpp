#pragma once
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <vector>
#include <mutex>
#include <atomic>
#include <thread>
#include <string>
#include <unordered_set>

#include "core/types.hpp"
#include "ui/theme.hpp"
#include "services/favorites.hpp"

// =============================================================================
//  Trading Platform — AppState
//  Central shared state passed to all modules.
// =============================================================================

struct AppLayout {
    float win_w = 1400.f;
    float win_h = 900.f;

    // ── Borderless window chrome ──────────────────────────────────────────────
    static constexpr float TITLEBAR_H    = 25.f;  // custom title-bar height
    static constexpr float APPTAB_H      = 36.f;  // global app-tab bar height
    static constexpr float RESIZE_BORDER = 6.f;   // resize hit-zone width

    // ── Toolbox strip on left edge of chart ──────────────────────────────────
    static constexpr float TOOLBOX_W = 36.f;

    // Proportion of width given to chart+toolbox vs right panel [0.4 – 0.85]
    float split_ratio  = 0.65f;
    // Proportion of right panel height given to market table vs ob/trades/alerts [0.2 – 0.8]
    float vsplit_ratio = 0.55f;

    // ── Content area (below titlebar + apptab bar) ────────────────────────────
    float content_y = TITLEBAR_H + APPTAB_H;
    float content_h = win_h - TITLEBAR_H - APPTAB_H;

    float chart_x = 0.f;
    float chart_y = TITLEBAR_H + APPTAB_H;
    float chart_w = 900.f;
    float chart_h = win_h - TITLEBAR_H - APPTAB_H;

    // Toolbox (left strip inside chart panel)
    float toolbox_x = 0.f;
    float toolbox_y = TITLEBAR_H + APPTAB_H;
    float toolbox_w = TOOLBOX_W;
    float toolbox_h = win_h - TITLEBAR_H - APPTAB_H;

    float table_x = 900.f;
    float table_y = TITLEBAR_H + APPTAB_H;
    float table_w = 700.f;
    float table_h = 500.f;

    float right_tabs_x = 900.f;
    float right_tabs_y = 500.f;
    float right_tabs_w = 700.f;
    float right_tabs_h = 400.f;

};

struct AppState {
    AppLayout layout;

    // ── SDL handles ──────────────────────────────────────────────────────────
    SDL_Window*   window   = nullptr;
    SDL_Renderer* renderer = nullptr;
    TTF_Font*     font      = nullptr;   // 13 pt
    TTF_Font*     font_xs   = nullptr;   // 9 pt
    TTF_Font*     font_sm   = nullptr;   // 11 pt
    TTF_Font*     font_lg   = nullptr;   // 16 pt
    TTF_Font*     font_icon = nullptr;   // sdl-trading.ttf icon glyphs (titlebar size)
    TTF_Font*     font_icon_sm = nullptr; // sdl-trading.ttf smaller (toolbox size)

    // Cursors
    SDL_Cursor* cursor_arrow       = nullptr;
    SDL_Cursor* cursor_hand        = nullptr;
    SDL_Cursor* cursor_cross       = nullptr;
    SDL_Cursor* cursor_resize_ns   = nullptr;
    SDL_Cursor* cursor_resize_ew   = nullptr;
    SDL_Cursor* cursor_resize_nwse = nullptr;  // ↘ NW-SE diagonal
    SDL_Cursor* cursor_resize_nesw = nullptr;  // ↗ NE-SW diagonal
    SDL_Cursor* cursor_move        = nullptr;

    // ── Chart state ──────────────────────────────────────────────────────────
    std::vector<Candle> candles;
    std::mutex          candles_mtx;
    std::string         chart_symbol  = "BTCUSDT";
    int                 tf_index      = 5;   // default 1h
    bool                candles_dirty = false;
    float               zoom_level    = 1.0f;
    int                 view_offset   = 0;
    float               price_zoom    = 1.0f;
    float               price_scroll  = 0.0f;
    bool                auto_scale    = true;
    float               mouse_x       = 0.f;
    float               mouse_y       = 0.f;
    bool                chart_dragging = false;
    bool                y_axis_dragging = false;
    float               drag_start_mx = 0.f;
    float               drag_start_my = 0.f;
    int                 drag_start_offset = 0;
    float               drag_start_price_scroll = 0.f;
    float               drag_start_price_zoom = 1.0f;
    float               drag_start_zoom_level = 1.0f;

    // ── Global navigation tab ─────────────────────────────────────────────────
    AppTab active_app_tab = AppTab::MARKETS;

    // ── Custom window management (borderless) ─────────────────────────────────
    bool  titlebar_dragging  = false;
    float titlebar_drag_ox   = 0.f;   // mouse offset within titlebar at drag start
    float titlebar_drag_oy   = 0.f;
    bool  win_maximized      = false;
    int   win_restore_x      = 100;   // pre-maximize position/size
    int   win_restore_y      = 100;
    int   win_restore_w      = 1400;
    int   win_restore_h      = 900;
    // Border resize dragging
    ResizeDir resize_dir           = ResizeDir::NONE;
    bool      border_resize_drag   = false;
    int       brd_drag_start_mx    = 0;
    int       brd_drag_start_my    = 0;
    int       brd_drag_start_wx    = 0;
    int       brd_drag_start_wy    = 0;
    int       brd_drag_start_ww    = 0;
    int       brd_drag_start_wh    = 0;

    // ── Aero Snap emulation ───────────────────────────────────────────────────
    bool      snap_drag_active = false;  // titlebar SDL-native drag is in progress
    SnapZone  snap_zone        = SnapZone::NONE;  // current hover snap zone
    int       snap_pre_x       = 0;    // window pos before snap drag started
    int       snap_pre_y       = 0;
    int       snap_pre_w       = 0;    // window size before snap drag started
    int       snap_pre_h       = 0;

    // ── Panel resize drag ─────────────────────────────────────────────────────
    bool  h_resize_dragging  = false;  // dragging vertical divider (chart|right panel)
    bool  v_resize_dragging  = false;  // dragging horizontal divider (table|orderbook)
    float resize_drag_start  = 0.f;    // starting mouse pos for drag
    float resize_drag_ratio  = 0.f;    // ratio snapshot at drag start

    // ── Toolbox & Drawing ─────────────────────────────────────────────────────
    enum class Tool { CURSOR, CROSSHAIR, TREND_LINE, TREND_LINE_EXT, HORIZ_LINE, TEXT_NOTE };
    Tool active_tool = Tool::CURSOR;

    struct ChartDrawing {
        Tool tool;
        int64_t t1 = 0;
        float p1 = 0.f;
        int64_t t2 = 0;
        float p2 = 0.f;
        std::string text;
    };
    std::vector<ChartDrawing> drawings;
    bool drawing_in_progress = false;
    ChartDrawing current_drawing;
    float hover_price = 0.f;
    int64_t hover_time = 0;

    // ── Market pairs ─────────────────────────────────────────────────────────
    std::vector<PairInfo> pairs_spot;
    std::vector<PairInfo> pairs_futures;
    std::mutex            pairs_mtx;
    bool                  pairs_dirty = false;

    MarketTab   active_tab   = MarketTab::SPOT;
    SortColumn  sort_col     = SortColumn::CHANGE_24H;
    SortDir     sort_dir     = SortDir::DESC;
    int         table_scroll = 0;
    int         table_hover  = -1;

    // ── Search ───────────────────────────────────────────────────────────────
    SearchState search;

    // ── Right panel ──────────────────────────────────────────────────────────
    RightTab right_tab = RightTab::ORDERBOOK;
    int      ob_scroll     = 0;   // scroll offset for order-book rows
    int      trades_scroll = 0;   // scroll offset for trades rows

    // Order book
    OrderBook orderbook;
    bool      ob_dirty = false;

    // Recent trades
    std::vector<Trade> trades;
    std::mutex         trades_mtx;
    bool               trades_dirty = false;

    // ── Alerts ───────────────────────────────────────────────────────────────
    std::vector<Alert> alerts;
    std::mutex         alerts_mtx;
    // Edit/create modal
    bool   alert_modal_open  = false;
    Alert  alert_modal_draft;           // draft being edited
    bool   alert_modal_is_new = true;   // true = create, false = edit
    int    alert_modal_field  = -1;     // which field is focused (-1 = none)
    std::string alert_threshold_buf;    // editable string for threshold field
    // Notification banner (auto-fade after 4 s)
    std::string notification_text;
    int64_t     notification_ts = 0;    // epoch ms when triggered

    // ── Favorites ────────────────────────────────────────────────────────────
    FavoritesState favorites;

    // ── Account Simulation ───────────────────────────────────────────────────
    AccountInfo account;
    std::mutex  account_mtx;

    // ── Theme ────────────────────────────────────────────────────────────────
    Theme       theme        = Theme::SYSTEM;
    ThemeColors theme_colors;   // resolved at start and on toggle

    // ── Lifecycle ────────────────────────────────────────────────────────────
    std::atomic<bool> running{true};
    std::thread candle_thread;
    std::thread pairs_thread;
    std::thread ob_thread;
    std::thread trades_thread;
    std::thread alerts_thread;
    std::thread account_thread;
};
