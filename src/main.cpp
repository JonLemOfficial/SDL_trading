// =============================================================================
//  Trading Platform C++ — SDL3
//  Entry point / SDL3 callback orchestrator.
//
//  Architecture:
//    main.cpp          ← SDL3 AppInit / AppEvent / AppIterate / AppQuit
//    chart.*           ← Candlestick chart + candle-fetch thread
//    market_table.*    ← Market pairs table (SPOT / FUTURES / FAV)
//    orderbook.*       ← Live order book panel + fetch thread
//    trades.*          ← Recent trades panel + fetch thread
//    alerts.*          ← Alert system, engine thread, modal
//    right_panel.*     ← OB / Trades / Alerts tab switcher
//    theme.*           ← Dark / Light / System colour palettes
//    ui_utils.*        ← Draw primitives, formatters
//    network.*         ← HTTP GET/POST, Telegram
//    favorites.*       ← Favourite symbol persistence
//    types.hpp         ← Shared data structures
//    app_state.hpp     ← Central AppState
//    constants.hpp     ← Window / path constants
// =============================================================================

#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <thread>
#include <chrono>
#include <algorithm>
#include <string>
#include <sstream>

#include "constants.hpp"
#include "types.hpp"
#include "app_state.hpp"
#include "theme.hpp"
#include "ui_utils.hpp"
#include "network.hpp"
#include "favorites.hpp"
#include "chart.hpp"
#include "market_table.hpp"
#include "orderbook.hpp"
#include "trades.hpp"
#include "alerts.hpp"
#include "right_panel.hpp"

// =============================================================================
//  Notification banner
// =============================================================================


static void update_layout(AppState* state) {
    // SDL_SetRenderLogicalPresentation pins us to 1600×900 logical space.
    constexpr float W = 1600.f;
    constexpr float H = 900.f;
    constexpr float DIV = 4.f;  // divider thickness

    state->layout.win_w = W;
    state->layout.win_h = H;

    // Horizontal split: left side (chart) | right side (table + right-panel)
    float split_x = W * std::max(0.40f, std::min(0.85f, state->layout.split_ratio));

    state->layout.chart_x = 0.f;
    state->layout.chart_y = 0.f;
    state->layout.chart_w = split_x;
    state->layout.chart_h = H;

    // Toolbox strip (leftmost slice of chart panel)
    state->layout.toolbox_x = 0.f;
    state->layout.toolbox_y = 0.f;
    state->layout.toolbox_w = AppLayout::TOOLBOX_W;
    state->layout.toolbox_h = H;

    float right_x = split_x + DIV;
    float right_w = W - right_x;

    // Vertical split inside right panel (market table | order-book / trades / alerts)
    float vsplit_y = H * std::max(0.20f, std::min(0.80f, state->layout.vsplit_ratio));

    state->layout.table_x    = right_x;
    state->layout.table_y    = 0.f;
    state->layout.table_w    = right_w;
    state->layout.table_h    = vsplit_y;

    state->layout.right_tabs_x = right_x;
    state->layout.right_tabs_y = vsplit_y + DIV;
    state->layout.right_tabs_w = right_w;
    state->layout.right_tabs_h = H - vsplit_y - DIV;
}

// =============================================================================
//  Toolbox rendering
// =============================================================================
static void draw_toolbox(AppState* state) {
    SDL_Renderer* r = state->renderer;
    const ThemeColors& tc = state->theme_colors;

    float tx = state->layout.toolbox_x;
    float ty = state->layout.toolbox_y;
    float tw = state->layout.toolbox_w;
    float th = state->layout.toolbox_h;

    ui_fill_rect(r, tx, ty, tw, th, tc.bg_panel);
    // Right-side separator line (between toolbox and chart)
    ui_draw_line(r, tx + tw - 1.f, ty, tx + tw - 1.f, ty + th, tc.border);

    using Tool = AppState::Tool;
    struct ToolBtn { const char* icon; const char* label; Tool t; };
    static const ToolBtn btns[] = {
        { "V",  "Cursor",       Tool::CURSOR         },
        { "+",  "Crosshair",    Tool::CROSSHAIR      },
        { "/",  "Trend Line",   Tool::TREND_LINE     },
        { "//", "Extended Tl.", Tool::TREND_LINE_EXT },
        { "=",  "Horiz Line",   Tool::HORIZ_LINE     },
        { "T",  "Text Note",    Tool::TEXT_NOTE      },
    };
    constexpr int   NUM_TOOLS = 6;
    constexpr float ICON_H   = 34.f;
    constexpr float ICON_PAD = 2.f;

    int hovered_idx = -1;
    float hovered_by = 0;

    for (int i = 0; i < NUM_TOOLS; i++) {
        float by = ty + 6.f + i * (ICON_H + ICON_PAD);
        bool active = (state->active_tool == btns[i].t);
        bool hovered = (state->mouse_x >= tx && state->mouse_x < tx + tw &&
                        state->mouse_y >= by && state->mouse_y < by + ICON_H);
        
        if (hovered) {
            hovered_idx = i;
            hovered_by = by;
        }

        SDL_Color bg = active  ? tc.bg_btn_active
                     : hovered ? tc.bg_btn_idle
                     : tc.bg_panel;
        SDL_Color fg = active ? tc.text_primary : tc.text_muted;
        
        float bx = tx + 2.f;
        float bw = tw - 4.f;
        ui_fill_rect(r, bx, by, bw, ICON_H, bg);
        ui_draw_rect(r, bx, by, bw, ICON_H, active ? tc.border_accent : tc.border);
        
        // Draw vectorized icon
        float cx = bx + bw * 0.5f;
        float cy = by + ICON_H * 0.5f;
        
        if (btns[i].t == Tool::CURSOR) {
            ui_draw_line(r, cx - 4, cy + 4, cx - 4, cy - 6, fg);
            ui_draw_line(r, cx - 4, cy - 6, cx + 4, cy + 2, fg);
            ui_draw_line(r, cx + 4, cy + 2, cx - 1, cy + 2, fg);
            ui_draw_line(r, cx - 1, cy + 2, cx - 4, cy + 4, fg);
        } else if (btns[i].t == Tool::CROSSHAIR) {
            ui_draw_line(r, cx - 6, cy, cx + 6, cy, fg);
            ui_draw_line(r, cx, cy - 6, cx, cy + 6, fg);
        } else if (btns[i].t == Tool::TREND_LINE) {
            ui_draw_line(r, cx - 6, cy + 6, cx + 6, cy - 6, fg);
            ui_fill_rect(r, cx - 7, cy + 5, 3, 3, fg);
            ui_fill_rect(r, cx + 5, cy - 7, 3, 3, fg);
        } else if (btns[i].t == Tool::TREND_LINE_EXT) {
            // Extended line: dots at edges, line crossing full width
            ui_draw_line(r, cx - 8, cy + 8, cx + 8, cy - 8, fg);
            ui_fill_rect(r, cx - 9, cy + 7, 2, 2, fg);
            ui_fill_rect(r, cx + 7, cy - 9, 2, 2, fg);
            // Small arrows at edges to indicate extension
            ui_draw_line(r, cx - 7, cy + 6, cx - 10, cy + 9, fg);
            ui_draw_line(r, cx + 7, cy - 8, cx + 10, cy - 11, fg);
        } else if (btns[i].t == Tool::HORIZ_LINE) {
            ui_draw_line(r, cx - 8, cy, cx + 8, cy, fg);
            ui_fill_rect(r, cx - 2, cy - 1, 4, 3, fg);
        } else if (btns[i].t == Tool::TEXT_NOTE) {
            ui_draw_line(r, cx - 5, cy - 5, cx + 5, cy - 5, fg);
            ui_draw_line(r, cx, cy - 5, cx, cy + 6, fg);
            ui_draw_line(r, cx - 2, cy + 6, cx + 2, cy + 6, fg);
            ui_draw_line(r, cx - 5, cy - 5, cx - 5, cy - 3, fg);
            ui_draw_line(r, cx + 5, cy - 5, cx + 5, cy - 3, fg);
        }
    }

    // ── "Clear All" button at bottom of toolbox ───────────────────────────────
    constexpr float CLR_H = 28.f;
    float clr_y = ty + th - CLR_H - 6.f;
    float clr_bx = tx + 2.f;
    float clr_bw = tw - 4.f;
    bool clr_hovered = (state->mouse_x >= tx && state->mouse_x < tx + tw &&
                        state->mouse_y >= clr_y && state->mouse_y < clr_y + CLR_H);
    SDL_Color clr_bg = clr_hovered ? tc.bear : tc.bg_btn_idle;
    SDL_Color clr_fg = clr_hovered ? tc.text_primary : tc.text_muted;
    ui_fill_rect(r, clr_bx, clr_y, clr_bw, CLR_H, clr_bg);
    ui_draw_rect(r, clr_bx, clr_y, clr_bw, CLR_H, tc.border);
    // "X" icon
    float cc = clr_bx + clr_bw * 0.5f;
    float cm = clr_y + CLR_H * 0.5f;
    ui_draw_line(r, cc - 5, cm - 5, cc + 5, cm + 5, clr_fg);
    ui_draw_line(r, cc + 5, cm - 5, cc - 5, cm + 5, clr_fg);

    // Draw tooltip for hovered_idx OR Clear All
    if (hovered_idx >= 0) {
        const char* label = btns[hovered_idx].label;
        int txt_w = ui_text_width(state->font_sm, label);
        float tt_x = tx + tw + 6.f;
        float tt_y = hovered_by + (ICON_H - 20.f) * 0.5f;
        ui_fill_rect(r, tt_x, tt_y, txt_w + 12.f, 20.f, tc.bg_header);
        ui_draw_rect(r, tt_x, tt_y, txt_w + 12.f, 20.f, tc.border);
        ui_draw_text(r, state->font_sm, tt_x + 6.f, tt_y + 3.f, label, tc.text_primary);
    } else if (clr_hovered) {
        int txt_w = ui_text_width(state->font_sm, "Clear All");
        float tt_x = tx + tw + 6.f;
        float tt_y = clr_y + (CLR_H - 20.f) * 0.5f;
        ui_fill_rect(r, tt_x, tt_y, txt_w + 12.f, 20.f, tc.bg_header);
        ui_draw_rect(r, tt_x, tt_y, txt_w + 12.f, 20.f, tc.border);
        ui_draw_text(r, state->font_sm, tt_x + 6.f, tt_y + 3.f, "Clear All", tc.text_primary);
    }
}


// =============================================================================
//  Resize dividers rendering
// =============================================================================
static void draw_dividers(AppState* state) {
    SDL_Renderer* r = state->renderer;
    const ThemeColors& tc = state->theme_colors;
    constexpr float DIV = 4.f;

    // Vertical divider — between chart and right panel
    float vdiv_x = state->layout.chart_w;
    ui_fill_rect(r, vdiv_x, 0.f, DIV, state->layout.win_h, tc.bg_header);

    // Horizontal divider — between table and order-book/trades/alerts
    float hdiv_y = state->layout.table_h;
    float rx     = state->layout.table_x;
    float rw     = state->layout.table_w;
    ui_fill_rect(r, rx, hdiv_y, rw, DIV, tc.bg_header);
}

static void draw_notification(AppState* state) {
    if (state->notification_text.empty()) return;
    int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    int64_t elapsed = now_ms - state->notification_ts;
    if (elapsed > 4000) {
        state->notification_text.clear();
        return;
    }
    // Fade out last second
    Uint8 alpha = (elapsed < 3000) ? 230 : (Uint8)(230 * (1.f - (elapsed - 3000) / 1000.f));
    SDL_Renderer* r = state->renderer;
    const ThemeColors& tc = state->theme_colors;

    float nw = 400.f, nh = 60.f;
    float nx = ((float)state->layout.win_w - nw) * 0.5f;
    float ny = 20.f;

    SDL_Color nbg = tc.notification_bg;
    nbg.a = alpha;
    ui_fill_rect(r, nx, ny, nw, nh, nbg);
    SDL_Color nbrd = tc.border_accent;
    nbrd.a = alpha;
    ui_draw_rect(r, nx, ny, nw, nh, nbrd);

    SDL_Color ntxt = tc.text_primary;
    ntxt.a = alpha;
    // First line only (truncate multi-line)
    std::string first_line = state->notification_text;
    auto nl = first_line.find('\n');
    if (nl != std::string::npos) first_line = first_line.substr(0, nl);
    ui_draw_text_centered(r, state->font_sm, nx, ny, nw, nh,
                          first_line.c_str(), ntxt);
}

// =============================================================================
//  SDL3 Callbacks
// =============================================================================

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO))             return SDL_APP_FAILURE;
    if (!TTF_Init())                           return SDL_APP_FAILURE;
    if (curl_global_init(CURL_GLOBAL_DEFAULT)) return SDL_APP_FAILURE;

    AppState* state = new AppState();
    *appstate = state;

    // Resolve theme early (needed for initial colour palette)
    state->theme        = Theme::SYSTEM;
    state->theme_colors = get_theme(state->theme);

    if (!SDL_CreateWindowAndRenderer(
            "Trading Platform C++ — SDL3",
            state->layout.win_w, state->layout.win_h,
            SDL_WINDOW_RESIZABLE,
            &state->window, &state->renderer))
        return SDL_APP_FAILURE;

    SDL_SetRenderDrawBlendMode(state->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderLogicalPresentation(state->renderer, 1600, 900, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    state->font    = TTF_OpenFont(FONT_PATH, 13);
    state->font_sm = TTF_OpenFont(FONT_PATH, 11);
    state->font_lg = TTF_OpenFont(FONT_PATH, 16);
    if (!state->font || !state->font_sm || !state->font_lg) {
        SDL_Log("Error: Could not open font at %s", FONT_PATH);
        return SDL_APP_FAILURE;
    }

    // Load persisted data
    fav_load(state->favorites, FAVORITES_FILE);
    alerts_load(state->alerts, ALERTS_FILE);

    // Start background threads
    state->candle_thread = std::thread(fetch_candles, state);
    state->pairs_thread  = std::thread(fetch_pairs,   state);
    state->ob_thread     = std::thread(fetch_orderbook, state);
    state->trades_thread = std::thread(fetch_trades,  state);
    state->alerts_thread = std::thread(alerts_engine, state);

    state->cursor_arrow = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
    state->cursor_hand = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
    state->cursor_cross = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
    state->cursor_resize_ns = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NS_RESIZE);
    state->cursor_resize_ew = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_EW_RESIZE);
    state->cursor_move = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_MOVE);


    return SDL_APP_CONTINUE;
}

// ── Event handler ─────────────────────────────────────────────────────────────

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    AppState* state = (AppState*)appstate;
    SDL_ConvertEventToRenderCoordinates(state->renderer, event);
    update_layout(state);

    if (event->type == SDL_EVENT_QUIT) return SDL_APP_SUCCESS;
    if (event->type == SDL_EVENT_KEY_DOWN) {
        // Modal input handling
        if (state->alert_modal_open) {
            if (event->key.key == SDLK_ESCAPE) {
                state->alert_modal_open = false;
            }
            // Text input for modal is handled in SDL_EVENT_TEXT_INPUT below
            return SDL_APP_CONTINUE;
        }

        if (event->key.key == SDLK_ESCAPE) return SDL_APP_SUCCESS;

        // Ctrl+F → focus search
        if (event->key.key == SDLK_F &&
            (SDL_GetModState() & SDL_KMOD_CTRL)) {
            state->search.focused = true;
            return SDL_APP_CONTINUE;
        }

        // Search bar key input
        if (state->search.focused) {
            if (event->key.key == SDLK_BACKSPACE && !state->search.query.empty())
                state->search.query.pop_back();
            if (event->key.key == SDLK_ESCAPE || event->key.key == SDLK_RETURN)
                state->search.focused = false;
            return SDL_APP_CONTINUE;
        }
    }

    if (event->type == SDL_EVENT_TEXT_INPUT) {
        if (state->search.focused) {
            state->search.query += event->text.text;
            state->table_scroll = 0;
        }
        // TODO: modal field input is handled similarly
        return SDL_APP_CONTINUE;
    }

    if (event->type == SDL_EVENT_MOUSE_WHEEL) {
        if (state->alert_modal_open) return SDL_APP_CONTINUE;

        float mx = event->wheel.mouse_x, my = event->wheel.mouse_y;

        if (mx < state->layout.chart_w) {
            // Chart side — zoom horizontally, or vertically over price axis
            float chart_x = state->layout.chart_x + AppLayout::TOOLBOX_W + 4.f;
            float chart_w = state->layout.chart_w - AppLayout::TOOLBOX_W - 14.f;
            float chart_area_x = chart_x + 10.f;
            float chart_area_w = chart_w - 95.f;
            if (mx > chart_area_x + chart_area_w) {
                // Over price axis: zoom vertically
                float factor = (event->wheel.y > 0) ? 1.1f : 0.9f;
                state->price_zoom *= factor;
                state->auto_scale = false;
            } else {
                // Over chart: zoom horizontally
                float factor = (event->wheel.y > 0) ? 1.18f : 0.85f;
                state->zoom_level *= factor;
                state->zoom_level  = std::max(0.1f, std::min(state->zoom_level, 20.f));
            }
        } else {
            // Right side — route to right-panel content or market table
            float rpx, rpy, rpw, rph;
            right_panel_content_rect(state, rpx, rpy, rpw, rph);
            if (point_in(mx, my, rpx, rpy, rpw, rph)) {
                // Scrolling inside right-panel content (OB / Trades / Alerts)
                switch (state->right_tab) {
                    case RightTab::ORDERBOOK:
                        state->ob_scroll -= (int)event->wheel.y * 2;
                        state->ob_scroll  = std::max(0, state->ob_scroll);
                        break;
                    case RightTab::TRADES:
                        state->trades_scroll -= (int)event->wheel.y * 2;
                        state->trades_scroll  = std::max(0, state->trades_scroll);
                        break;
                    default: break;
                }
            } else {
                // Scrolling in market table area
                state->table_scroll -= (int)event->wheel.y * 3;
                state->table_scroll  = std::max(0, state->table_scroll);
            }
        }
    }


    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        float mx = (float)event->button.x;
        float my = (float)event->button.y;

        if (event->button.button == SDL_BUTTON_LEFT) {
            // ── Divider resize (highest priority) ────────────────────────────
            constexpr float HIT = 6.f;
            // Vertical divider: chart | right panel
            if (std::abs(mx - state->layout.chart_w) < HIT) {
                state->h_resize_dragging = true;
                state->resize_drag_start = mx;
                state->resize_drag_ratio = state->layout.split_ratio;
                return SDL_APP_CONTINUE;
            }
            // Horizontal divider: table | right-tabs
            if (mx >= state->layout.table_x &&
                mx <= state->layout.table_x + state->layout.table_w &&
                std::abs(my - state->layout.table_h) < HIT) {
                state->v_resize_dragging = true;
                state->resize_drag_start = my;
                state->resize_drag_ratio = state->layout.vsplit_ratio;
                return SDL_APP_CONTINUE;
            }

            // Double-click on price axis → reset auto-scale
            if (event->button.clicks >= 2 && mx < state->layout.chart_w) {
                float tbw        = AppLayout::TOOLBOX_W;
                float cx_off     = state->layout.chart_x + tbw + 4.f;
                float cw_off     = state->layout.chart_w  - tbw - 14.f;
                float chart_ax   = cx_off + 10.f;
                float chart_aw   = cw_off - 95.f;
                if (mx > chart_ax + chart_aw) {
                    state->auto_scale   = true;
                    state->price_zoom   = 1.0f;
                    state->price_scroll = 0.0f;
                }
            }
        }



        // ── Alert modal click handling ────────────────────────────────────────
        if (state->alert_modal_open) {
            float mw = 480.f, mh = 420.f;
            float modal_x = ((float)state->layout.win_w  - mw) * 0.5f;
            float modal_y = ((float)state->layout.win_h - mh) * 0.5f;
            float btn_w = 100.f, btn_h = 28.f;
            float btn_y = modal_y + mh - btn_h - 10.f;

            // Cancel
            if (point_in(mx, my, modal_x + 10.f, btn_y, btn_w, btn_h)) {
                state->alert_modal_open = false;
                return SDL_APP_CONTINUE;
            }
            // Save
            if (point_in(mx, my, modal_x + mw - btn_w - 10.f, btn_y, btn_w, btn_h)) {
                std::lock_guard<std::mutex> lock(state->alerts_mtx);
                if (state->alert_modal_is_new) {
                    state->alert_modal_draft.id = alerts_new_id();
                    state->alerts.push_back(state->alert_modal_draft);
                } else {
                    for (auto& a : state->alerts)
                        if (a.id == state->alert_modal_draft.id)
                            a = state->alert_modal_draft;
                }
                alerts_save(state->alerts, ALERTS_FILE);
                state->alert_modal_open = false;
                return SDL_APP_CONTINUE;
            }

            // Configuration Buttons
            handle_alert_modal_click(state, mx, my);
            return SDL_APP_CONTINUE; // swallow all other modal clicks
        }

        // ── Chart area clicks ─────────────────────────────────────────────────
        if (mx < state->layout.chart_w) {
            // ── Toolbox buttons (leftmost strip) ─────────────────────────────
            float tbw = AppLayout::TOOLBOX_W;
            if (mx < tbw) {
                using Tool = AppState::Tool;
                static const Tool tool_order[] = {
                    Tool::CURSOR, Tool::CROSSHAIR, Tool::TREND_LINE,
                    Tool::TREND_LINE_EXT, Tool::HORIZ_LINE, Tool::TEXT_NOTE
                };
                constexpr int   NUM_TB   = 6;
                constexpr float ICON_H   = 34.f;
                constexpr float ICON_PAD = 2.f;
                for (int i = 0; i < NUM_TB; i++) {
                    float by = 6.f + i * (ICON_H + ICON_PAD);
                    if (point_in(mx, my, 2.f, by, tbw - 4.f, ICON_H)) {
                        state->active_tool = tool_order[i];
                        return SDL_APP_CONTINUE;
                    }
                }
                // Clear All button at the bottom of toolbox
                constexpr float CLR_H = 28.f;
                float clr_y = state->layout.toolbox_h - CLR_H - 6.f;
                if (point_in(mx, my, 2.f, clr_y, tbw - 4.f, CLR_H)) {
                    state->drawings.clear();
                    state->drawing_in_progress = false;
                    return SDL_APP_CONTINUE;
                }
                return SDL_APP_CONTINUE;
            }

            // Chart panel local coordinates (offset by toolbox)
            float cx_off  = state->layout.chart_x + tbw + 4.f;
            float cy_off  = state->layout.chart_y + 10.f;
            float cw_off  = state->layout.chart_w - tbw - 14.f;

            // Timeframe buttons
            float btn_x_start = cx_off + cw_off - (float)(NUM_TIMEFRAMES * 54 + 10);
            float btn_y_v     = cy_off + 6.f;
            for (int i = 0; i < NUM_TIMEFRAMES; i++) {
                float bx = btn_x_start + i * 54.f;
                if (point_in(mx, my, bx, btn_y_v, 50.f, 24.f)) {
                    state->tf_index    = i;
                    state->view_offset = 0;
                    std::lock_guard<std::mutex> lk(state->candles_mtx);
                    state->candles.clear();
                    return SDL_APP_CONTINUE;
                }
            }

            // Chart drawing area hit-test
            float chart_area_x = cx_off + 10.f;
            float chart_area_y = cy_off + 46.f;
            float chart_area_w = cw_off - 95.f;
            float chart_area_h = (state->layout.chart_h - 20.f) - 80.f;
            if (point_in(mx, my, chart_area_x, chart_area_y, chart_area_w, chart_area_h)) {
                if (state->active_tool != AppState::Tool::CURSOR && state->active_tool != AppState::Tool::CROSSHAIR) {
                    state->drawing_in_progress = true;
                    state->current_drawing.tool = state->active_tool;
                    state->current_drawing.t1 = state->hover_time;
                    state->current_drawing.p1 = state->hover_price;
                    state->current_drawing.t2 = state->hover_time;
                    state->current_drawing.p2 = state->hover_price;
                    if (state->active_tool == AppState::Tool::TEXT_NOTE) {
                        state->current_drawing.text = "Note";
                    }
                } else {
                    state->chart_dragging = true;
                    state->drag_start_mx  = mx;
                    state->drag_start_my  = my;
                    state->drag_start_offset = state->view_offset;
                    state->drag_start_price_scroll = state->price_scroll;
                    state->auto_scale = false;
                }
            } else if (point_in(mx, my, chart_area_x + chart_area_w, chart_area_y, 95.f, chart_area_h)) {
                state->y_axis_dragging = true;
                state->drag_start_my   = my;
                state->drag_start_price_zoom = state->price_zoom;
                state->auto_scale = false;
            }
            return SDL_APP_CONTINUE;
        }


        // ── Right panel area clicks ────────────────────────────────────────────
        // (table_x == chart_w + divider, everything >= table_x is right side)
        if (mx >= state->layout.table_x) {

            // Right-panel tabs: ORDER BOOK | TRADES | ALERTS
            int rtab = hit_right_tab(state, mx, my);
            if (rtab >= 0) {
                state->right_tab     = (RightTab)rtab;
                state->ob_scroll     = 0;   // reset scroll on tab switch
                state->trades_scroll = 0;
                return SDL_APP_CONTINUE;
            }

            // Alerts add / row actions
            if (state->right_tab == RightTab::ALERTS) {
                float rpx, rpy, rpw, rph;
                right_panel_content_rect(state, rpx, rpy, rpw, rph);
                if (hit_alerts_add_button(mx, my, rpx, rpy, rpw)) {
                    state->alert_modal_is_new  = true;
                    state->alert_modal_draft   = Alert{};
                    state->alert_modal_draft.symbol = state->chart_symbol;
                    state->alert_modal_open    = true;
                    return SDL_APP_CONTINUE;
                }
                std::lock_guard<std::mutex> lock(state->alerts_mtx);
                int count = (int)state->alerts.size();
                int ridx = hit_alert_row(mx, my, rpx, rpy, rpw, rph, count);
                if (ridx >= 0 && ridx < count) {
                    float row_y = rpy + 22.f + 24.f + 4.f + ridx * 58.f;
                    if (hit_alert_del_btn(mx, my, rpx, row_y, rpw)) {
                        state->alerts.erase(state->alerts.begin() + ridx);
                        alerts_save(state->alerts, ALERTS_FILE);
                    } else if (hit_alert_toggle_btn(mx, my, rpx, row_y)) {
                        state->alerts[ridx].enabled = !state->alerts[ridx].enabled;
                        alerts_save(state->alerts, ALERTS_FILE);
                    }
                }
                return SDL_APP_CONTINUE;
            }

        // Theme toggle
            if (hit_theme_button(state, mx, my)) {
                state->theme        = theme_next(state->theme);
                state->theme_colors = get_theme(state->theme);
                return SDL_APP_CONTINUE;
            }

            // Search bar focus
            if (hit_search_bar(state, mx, my)) {
                state->search.focused = true;
                SDL_StartTextInput(state->window);
                return SDL_APP_CONTINUE;
            } else if (state->search.focused) {
                state->search.focused = false;
                SDL_StopTextInput(state->window);
            }

            // Market tabs (SPOT / FUTURES / FAV)
            int tab = hit_tab(state, mx, my);
            if (tab >= 0) {
                state->active_tab   = (MarketTab)tab;
                state->table_scroll = 0;
                return SDL_APP_CONTINUE;
            }

            // Column header sort
            int col = hit_col_header(state, mx, my);
            if (col >= 0) {
                SortColumn sc_map[] = {
                    SortColumn::NAME, SortColumn::PRICE, SortColumn::CHANGE_24H,
                    SortColumn::VOLUME_24H, SortColumn::HIGH_24H, SortColumn::LOW_24H
                };
                SortColumn new_sc = sc_map[col];
                if (state->sort_col == new_sc)
                    state->sort_dir = (state->sort_dir == SortDir::ASC) ? SortDir::DESC : SortDir::ASC;
                else { state->sort_col = new_sc; state->sort_dir = SortDir::DESC; }
                state->table_scroll = 0;
                return SDL_APP_CONTINUE;
            }

            // Row click — select symbol or toggle favourite
            auto sorted = get_sorted_filtered_pairs(state);
            int row = hit_table_row(mx, my, state, sorted);
            if (row >= 0 && row < (int)sorted.size()) {
                int row_idx = row - state->table_scroll;
                if (hit_fav_star(state, mx, my, row_idx)) {
                    fav_toggle(state->favorites, sorted[row].symbol);
                    fav_save(state->favorites, FAVORITES_FILE);
                } else {
                    std::string new_sym = sorted[row].symbol;
                    if (new_sym != state->chart_symbol) {
                        state->chart_symbol = new_sym;
                        state->view_offset  = 0;
                        std::lock_guard<std::mutex> lk(state->candles_mtx);
                        state->candles.clear();
                    }
                }
            }
        } // end right side
    }

    if (event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
        if (event->button.button == SDL_BUTTON_LEFT) {
            if (state->drawing_in_progress) {
                state->drawings.push_back(state->current_drawing);
                state->drawing_in_progress = false;
                // Optional: reset tool to cursor after drawing (commented out to allow multiple)
                // state->active_tool = AppState::Tool::CURSOR;
            }
            state->chart_dragging    = false;
            state->y_axis_dragging   = false;
            state->h_resize_dragging = false;
            state->v_resize_dragging = false;
        }
    }

    if (event->type == SDL_EVENT_MOUSE_MOTION) {
        float mx = (float)event->motion.x;
        float my = (float)event->motion.y;
        state->mouse_x = mx;
        state->mouse_y = my;

        // ── Modal Cursor handling ──────────────────────────────────────────────
        if (state->alert_modal_open) {
            update_alert_modal_cursor(state, mx, my);
            return SDL_APP_CONTINUE;
        }

        // ── Panel resize drags ────────────────────────────────────────────────
        if (state->h_resize_dragging) {
            float delta = mx - state->resize_drag_start;
            state->layout.split_ratio = std::max(0.40f, std::min(0.85f,
                state->resize_drag_ratio + delta / state->layout.win_w));
            SDL_SetCursor(state->cursor_resize_ew);
            return SDL_APP_CONTINUE;
        }
        if (state->v_resize_dragging) {
            float delta = my - state->resize_drag_start;
            state->layout.vsplit_ratio = std::max(0.20f, std::min(0.80f,
                state->resize_drag_ratio + delta / state->layout.win_h));
            SDL_SetCursor(state->cursor_resize_ns);
            return SDL_APP_CONTINUE;
        }

        // ── Chart drags & drawing ───────────────────────────────────────────────
        if (state->drawing_in_progress) {
            state->current_drawing.t2 = state->hover_time;
            state->current_drawing.p2 = state->hover_price;
        } else if (state->chart_dragging) {
            float dx = mx - state->drag_start_mx;
            float dy = my - state->drag_start_my;
            int visible = (int)(80.f * state->zoom_level);
            float tbw     = AppLayout::TOOLBOX_W;
            float chart_w = state->layout.chart_w - tbw - 14.f - 95.f;  // drawable width minus price axis
            float candle_w = chart_w / (float)visible;
            int offset_shift = (int)(dx / candle_w);
            state->view_offset = std::max(0, state->drag_start_offset + offset_shift);
            float chart_h = (state->layout.chart_h - 20.f) - 80.f;
            state->price_scroll = state->drag_start_price_scroll + (dy / chart_h);
        } else if (state->y_axis_dragging) {
            float dy = my - state->drag_start_my;
            float factor = std::exp(-dy * 0.01f);
            state->price_zoom = state->drag_start_price_zoom * factor;
            state->price_zoom = std::max(0.01f, std::min(state->price_zoom, 100.f));
        }

        // ── Cursor shape based on position ────────────────────────────────────
        constexpr float HIT = 6.f;
        float tbw        = AppLayout::TOOLBOX_W;
        float cx_off     = state->layout.chart_x + tbw + 4.f;
        float cy_off     = state->layout.chart_y + 10.f;
        float cw_off     = state->layout.chart_w  - tbw - 14.f;
        float chart_ax   = cx_off + 10.f;
        float chart_ay   = cy_off + 46.f;
        float chart_aw   = cw_off - 95.f;
        float chart_ah   = (state->layout.chart_h - 20.f) - 80.f;

        if (std::abs(mx - state->layout.chart_w) < HIT) {
            SDL_SetCursor(state->cursor_resize_ew);
        } else if (mx >= state->layout.table_x &&
                   mx <= state->layout.table_x + state->layout.table_w &&
                   std::abs(my - state->layout.table_h) < HIT) {
            SDL_SetCursor(state->cursor_resize_ns);
        } else if (mx >= state->layout.table_x) {
            auto sorted = get_sorted_filtered_pairs(state);
            state->table_hover = hit_table_row(mx, my, state, sorted);

            bool hand = hit_theme_button(state, mx, my) || hit_search_bar(state, mx, my) ||
                        hit_tab(state, mx, my) >= 0 || hit_col_header(state, mx, my) >= 0 ||
                        hit_right_tab(state, mx, my) >= 0 || state->table_hover >= 0;

            // Alerts panel interactive elements
            if (!hand && state->right_tab == RightTab::ALERTS) {
                float rpx, rpy, rpw, rph;
                right_panel_content_rect(state, rpx, rpy, rpw, rph);
                if (hit_alerts_add_button(mx, my, rpx, rpy, rpw)) {
                    hand = true;
                } else {
                    std::lock_guard<std::mutex> lock(state->alerts_mtx);
                    int count = (int)state->alerts.size();
                    int ridx = hit_alert_row(mx, my, rpx, rpy, rpw, rph, count);
                    if (ridx >= 0 && ridx < count) {
                        float row_y = rpy + 22.f + 24.f + 4.f + ridx * 58.f;
                        if (hit_alert_del_btn(mx, my, rpx, row_y, rpw) ||
                            hit_alert_toggle_btn(mx, my, rpx, row_y)) {
                            hand = true;
                        }
                    }
                }
            }

            SDL_SetCursor(hand ? state->cursor_hand : state->cursor_arrow);
        } else if (mx < tbw) {
            // Toolbox strip — pointer cursor to indicate clickable buttons
            SDL_SetCursor(state->cursor_hand);
        } else {
            state->table_hover = -1;

            if (state->chart_dragging || state->y_axis_dragging) {
                SDL_SetCursor(state->cursor_move);
            } else if (mx > chart_ax + chart_aw && my >= chart_ay && my <= chart_ay + chart_ah) {
                SDL_SetCursor(state->cursor_resize_ns);
            } else if (point_in(mx, my, chart_ax, chart_ay, chart_aw, chart_ah)) {
                SDL_SetCursor(state->cursor_cross);
            } else if (my > chart_ay + chart_ah && mx >= chart_ax && mx <= chart_ax + chart_aw) {
                SDL_SetCursor(state->cursor_resize_ew);
            } else {
                SDL_SetCursor(state->cursor_arrow);
            }
        }



    }

    return SDL_APP_CONTINUE;
}

// ── Render loop ───────────────────────────────────────────────────────────────

SDL_AppResult SDL_AppIterate(void* appstate) {
    AppState* state = (AppState*)appstate;
    update_layout(state);
    SDL_Renderer* r = state->renderer;
    const ThemeColors& tc = state->theme_colors;

    SDL_SetRenderDrawColor(r, tc.bg_window.r, tc.bg_window.g, tc.bg_window.b, 255);
    SDL_RenderClear(r);

    draw_chart(state);
    draw_toolbox(state);
    draw_right_panel(state);
    draw_table(state);
    draw_dividers(state);
    draw_notification(state);

    if (state->alert_modal_open)
        draw_alert_modal(state);

    SDL_RenderPresent(r);
    return SDL_APP_CONTINUE;
}

// ── Shutdown ──────────────────────────────────────────────────────────────────

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    AppState* state = (AppState*)appstate;
    update_layout(state);
    if (!state) return;

    state->running = false;
    auto join = [](std::thread& t) { if (t.joinable()) t.join(); };
    join(state->candle_thread);
    join(state->pairs_thread);
    join(state->ob_thread);
    join(state->trades_thread);
    join(state->alerts_thread);

    // Save state
    fav_save(state->favorites, FAVORITES_FILE);
    {
        std::lock_guard<std::mutex> lock(state->alerts_mtx);
        alerts_save(state->alerts, ALERTS_FILE);
    }

    if (state->font_lg)  TTF_CloseFont(state->font_lg);
    if (state->font)     TTF_CloseFont(state->font);
    if (state->font_sm)  TTF_CloseFont(state->font_sm);
    if (state->renderer) SDL_DestroyRenderer(state->renderer);
    if (state->window)   SDL_DestroyWindow(state->window);

    
    if (state->cursor_arrow) SDL_DestroyCursor(state->cursor_arrow);
    if (state->cursor_hand) SDL_DestroyCursor(state->cursor_hand);
    if (state->cursor_cross) SDL_DestroyCursor(state->cursor_cross);
    if (state->cursor_resize_ns) SDL_DestroyCursor(state->cursor_resize_ns);
    if (state->cursor_resize_ew) SDL_DestroyCursor(state->cursor_resize_ew);
    if (state->cursor_move) SDL_DestroyCursor(state->cursor_move);

    delete state;
    TTF_Quit();
    curl_global_cleanup();
    SDL_Quit();
}
