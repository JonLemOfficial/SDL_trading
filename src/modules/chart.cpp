#include "modules/chart.hpp"
#include "services/network.hpp"
#include "ui/ui_utils.hpp"
#include "core/constants.hpp"

#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <thread>
#include <chrono>

using json = nlohmann::json;

// =============================================================================
//  Chart layout helpers (single source of truth for draw + click hit-test)
// =============================================================================

void chart_panel_rect(AppState* state, float& cx, float& cy, float& cw, float& ch) {
  const float TBW = AppLayout::TOOLBOX_W;
  cx = state->layout.chart_x + TBW + 4.f;
  cy = state->layout.chart_y + 10.f;
  cw = state->layout.chart_w - TBW - 14.f;
  ch = state->layout.chart_h - 20.f;
}

void chart_timeframe_btn_rect(AppState* state, int index,
                              float& bx, float& by, float& bw, float& bh) {
  float cx = 0.f, cy = 0.f, cw = 0.f, ch = 0.f;
  chart_panel_rect(state, cx, cy, cw, ch);
  (void)ch;

  bw = ChartTfBtnLayout::BTN_W;
  bh = ChartTfBtnLayout::BTN_H;
  float row_w = (float)NUM_TIMEFRAMES * bw +
                (float)(NUM_TIMEFRAMES - 1) * ChartTfBtnLayout::BTN_SPACING;
  float btn_x = cx + cw - row_w;
  float btn_y = cy + 6.f;
  bx = btn_x + (float)index * (bw + ChartTfBtnLayout::BTN_SPACING);
  by = btn_y;
}

int hit_chart_timeframe(AppState* state, float mx, float my) {
  for (int i = 0; i < NUM_TIMEFRAMES; i++) {
    float bx = 0.f, by = 0.f, bw = 0.f, bh = 0.f;
    chart_timeframe_btn_rect(state, i, bx, by, bw, bh);
    if (point_in(mx, my, bx, by, bw, bh))
      return i;
  }
  return -1;
}

// =============================================================================
//  Candle fetch thread
// =============================================================================

void fetch_candles(AppState* state) {
    while (state->running) {
        std::string symbol = state->chart_symbol;
        std::string tf     = TIMEFRAMES[state->tf_index];

        std::string url = "https://api.binance.com/api/v3/klines?symbol=" +
                          symbol + "&interval=" + tf + "&limit=300";
        std::string raw = http_get(url);

        if (!raw.empty()) {
            try {
                json j = json::parse(raw);
                std::vector<Candle> nc;
                nc.reserve(j.size());
                for (auto& item : j) {
                    Candle c;
                    c.open_time = item[0].get<int64_t>();
                    c.open   = std::stof(item[1].get<std::string>());
                    c.high   = std::stof(item[2].get<std::string>());
                    c.low    = std::stof(item[3].get<std::string>());
                    c.close  = std::stof(item[4].get<std::string>());
                    c.volume = std::stof(item[5].get<std::string>());
                    nc.push_back(c);
                }
                std::lock_guard<std::mutex> lock(state->candles_mtx);
                state->candles       = std::move(nc);
                state->candles_dirty = true;
            } catch (...) {}
        }

        // Sleep 5 s in 100 ms slices to react to running=false quickly
        for (int i = 0; i < 50 && state->running; i++)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// =============================================================================
//  Chart rendering
// =============================================================================

void draw_chart(AppState* state) {
    SDL_Renderer* r = state->renderer;
    const ThemeColors& tc = state->theme_colors;

    // Offset by toolbox width so chart panel doesn't render under the toolbox strip
    float cx = 0.f, cy = 0.f, cw = 0.f, ch = 0.f;
    chart_panel_rect(state, cx, cy, cw, ch);

    // Background panel
    ui_fill_rect(r, cx, cy, cw, ch, tc.bg_panel);
    ui_draw_rect(r, cx, cy, cw, ch, tc.border);

    // ── Title bar ────────────────────────────────────────────────────────────
    ui_fill_rect(r, cx, cy, cw, 36.f, tc.bg_header);
    std::string title = state->chart_symbol +
                        "  [" + TIMEFRAMES[state->tf_index] + "]  LIVE";
    ui_draw_text(r, state->font_lg, cx + 12.f, cy + 8.f, title.c_str(), tc.text_header);

    // ── Timeframe buttons ─────────────────────────────────────────────────────
    const float mr = 10.f;  // right margin
    for (int i = 0; i < NUM_TIMEFRAMES; i++) {
        float bx = 0.f - mr, by = 0.f, bw = 0.f, bh = 0.f;
        chart_timeframe_btn_rect(state, i, bx, by, bw, bh);

        bool active = (i == state->tf_index);
        SDL_Color bg  = active ? tc.bg_btn_active : tc.bg_tab_idle;
        SDL_Color txt = active ? tc.text_primary : tc.text_muted;
        bool tf_hovered = point_in(state->mouse_x, state->mouse_y, bx - 10.f, by, bw, bh);

        ui_fill_rect(r, bx - 10.f, by, bw, bh, tf_hovered ? tc.bg_btn_idle : bg);
        ui_draw_rect(r, bx - 10.f, by, bw, bh, tc.border);
        ui_draw_text_centered(r, state->font_sm, bx - 10.f, by, bw, bh, TIMEFRAMES[i], txt);
    }

    // ── Chart drawing area ────────────────────────────────────────────────────
    float chart_x = cx + 10.f;
    float chart_y = cy + 46.f;
    float chart_w = cw - 95.f;
    float chart_h = ch - 80.f;
    
    // Split chart into price (80%) and volume (20%)
    float price_h = chart_h * 0.8f;
    float vol_h   = chart_h * 0.2f;

    ui_fill_rect(r, chart_x, chart_y, chart_w, chart_h, tc.bg_window);

    std::lock_guard<std::mutex> lock(state->candles_mtx);
    if (state->candles.empty()) {
        ui_draw_text_centered(r, state->font, chart_x, chart_y, chart_w, chart_h,
                              "Loading candles...", tc.text_muted);
        return;
    }

    int visible = (int)(80.f * state->zoom_level);
    visible = std::max(10, std::min((int)state->candles.size(), visible));
    int total  = (int)state->candles.size();
    int offset = std::max(0, std::min(state->view_offset, total - visible));
    int start  = std::max(0, total - visible - offset);
    int end    = std::min(total, start + visible);

    // Price range
    float min_p = 1e18f, max_p = -1e18f;
    float max_vol = 0.f;
    for (int i = start; i < end; i++) {
        if (state->candles[i].low  < min_p) min_p = state->candles[i].low;
        if (state->candles[i].high > max_p) max_p = state->candles[i].high;
        if (state->candles[i].volume > max_vol) max_vol = state->candles[i].volume;
    }
    if (max_vol < 1e-6f) max_vol = 1.f;
    float vol_h_max = vol_h * 0.95f; // Add a small margin for volume bars

    float range = max_p - min_p;
    if (range < 1e-6f) range = 1.f;
    float padding = range * 0.06f;
    min_p -= padding; max_p += padding;
    range = max_p - min_p;

    float visible_mid_p = (max_p + min_p) * 0.5f;
    if (!state->auto_scale) {
        range /= state->price_zoom;
        visible_mid_p += range * state->price_scroll;
        min_p = visible_mid_p - range * 0.5f;
        max_p = visible_mid_p + range * 0.5f;
    }

    auto price_to_y = [&](float p) -> float {
        return chart_y + price_h - ((p - min_p) / range) * price_h;
    };

    // Y Grid lines + price labels (Horizontal)
    float target_y_spacing = 60.f;
    int approx_grid_lines = std::max(2, (int)(price_h / target_y_spacing));
    float approx_price_step = range / approx_grid_lines;
    
    float exponent = std::floor(std::log10(approx_price_step));
    float fraction = approx_price_step / std::pow(10.f, exponent);
    float nice_fraction = 1.f;
    if (fraction >= 7.5f) nice_fraction = 10.f;
    else if (fraction >= 3.5f) nice_fraction = 5.f;
    else if (fraction >= 1.5f) nice_fraction = 2.f;
    float price_step = nice_fraction * std::pow(10.f, exponent);

    float first_grid_p = std::ceil(min_p / price_step) * price_step;

    for (float p = first_grid_p; p <= max_p; p += price_step) {
        float gy = price_to_y(p);
        if (gy >= chart_y && gy <= chart_y + price_h) {
            ui_draw_line(r, chart_x, gy, chart_x + chart_w, gy, tc.grid);
            std::string pl = fmt_price(p);
            ui_draw_text(r, state->font_sm, chart_x + chart_w + 4.f, gy - 7.f,
                         pl.c_str(), tc.text_muted);
        }
    }

    // Candles
    float candle_w = chart_w / (float)visible;
    float body_w   = std::max(2.f, candle_w * 0.6f);

    for (int i = start; i < end; i++) {
        const auto& c  = state->candles[i];
        int   idx      = i - start;
        float cx_pos   = chart_x + idx * candle_w + candle_w * 0.5f;

        float hy  = price_to_y(c.high);
        float ly  = price_to_y(c.low);
        float oy  = price_to_y(c.open);
        float cly = price_to_y(c.close);

        bool  bull = c.close >= c.open;
        SDL_Color col = bull ? tc.bull : tc.bear;

        // Wick
        if (hy >= chart_y && ly <= chart_y + price_h) {
             ui_draw_line(r, cx_pos, std::max(chart_y, hy), cx_pos, std::min(chart_y + price_h, ly), col);
        }
        // Body
        float body_top = std::max(chart_y, std::min(oy, cly));
        float body_bot = std::min(chart_y + price_h, std::max(oy, cly));
        float body_h   = std::max(1.f, body_bot - body_top);
        if (body_bot > chart_y && body_top < chart_y + price_h) {
             ui_fill_rect(r, cx_pos - body_w * 0.5f, body_top, body_w, body_h, col);
        }
        
        // Volume bar
        float vol_bar_h = (c.volume / max_vol) * vol_h_max;
        SDL_Color vol_col = col;
        vol_col.a = 200; // Less transparent
        ui_fill_rect(r, cx_pos - body_w * 0.5f, chart_y + chart_h - vol_bar_h, body_w, vol_bar_h, vol_col);
    }

    // Current price dashed line + price tag
    float last_close = state->candles.back().close;
    float lc_y       = price_to_y(last_close);
    if (lc_y >= chart_y && lc_y <= chart_y + price_h) {
        SDL_SetRenderDrawColor(r, tc.accent.r, tc.accent.g, tc.accent.b, 200);
        for (float dx = chart_x; dx < chart_x + chart_w; dx += 8.f)
            SDL_RenderLine(r, dx, lc_y, dx + 4.f, lc_y);

        std::string lcp = fmt_price(last_close);
        float tag_w = (float)(ui_text_width(state->font_sm, lcp.c_str()) + 10);
        ui_fill_rect(r, chart_x + chart_w + 2.f, lc_y - 9.f, tag_w, 18.f, tc.price_tag_bg);
        ui_draw_text(r, state->font_sm, chart_x + chart_w + 5.f, lc_y - 7.f,
                     lcp.c_str(), tc.price_tag_text);
    }

    // Time labels (X axis) & Vertical grid lines
    int label_step = std::max(1, visible / 8);
    for (int i = start; i < end; i += label_step) {
        int64_t ms  = state->candles[i].open_time;
        bool date_only = (state->tf_index >= 7); // 1d, 1w
        std::string tl = fmt_time(ms, date_only ? 1 : 2); // 1=Date, 2=Date+Time
        float tx2 = chart_x + (i - start) * candle_w + candle_w * 0.5f;
        // X Grid line
        ui_draw_line(r, tx2, chart_y, tx2, chart_y + chart_h, tc.grid);
        // Label
        float lw = (float)ui_text_width(state->font_sm, tl.c_str());
        ui_draw_text(r, state->font_sm, tx2 - lw * 0.5f, chart_y + chart_h + 4.f,
                     tl.c_str(), tc.text_muted);
    }

    // Volume separator line
    ui_draw_line(r, chart_x, chart_y + price_h, chart_x + chart_w, chart_y + price_h, tc.grid);

    // Hint
    ui_draw_text(r, state->font_sm, cx + 12.f, cy + ch - 18.f,
                 "Drag: Pan | Scroll Y: Scale Price | Scroll X: Zoom | Dbl Click: Auto Scale", tc.text_muted);

    // Save mappings for main.cpp
    auto time_to_x = [&](int64_t t) -> float {
        // approximate by binary search (or just simple index math since candles are linear)
        // for simplicity, just loop over visible
        for (int i = start; i < end; i++) {
            if (state->candles[i].open_time >= t) {
                return chart_x + (i - start) * candle_w + candle_w * 0.5f;
            }
        }
        return chart_x + (end - start) * candle_w; // off screen right
    };

    // Draw user drawings
    auto draw_single = [&](const AppState::ChartDrawing& d, SDL_Color color) {
        if (d.tool == AppState::Tool::HORIZ_LINE) {
            float y = price_to_y(d.p1);
            if (y >= chart_y && y <= chart_y + price_h) {
                ui_draw_line(r, chart_x, y, chart_x + chart_w, y, color);
            }
        } else if (d.tool == AppState::Tool::TREND_LINE) {
            float x1 = time_to_x(d.t1);
            float y1 = price_to_y(d.p1);
            float x2 = time_to_x(d.t2);
            float y2 = price_to_y(d.p2);
            ui_draw_line(r, x1, y1, x2, y2, color);
        } else if (d.tool == AppState::Tool::TREND_LINE_EXT) {
            // Extended trend line: extrapolate from (x1,y1)→(x2,y2) across the full chart width
            float x1 = time_to_x(d.t1);
            float y1 = price_to_y(d.p1);
            float x2 = time_to_x(d.t2);
            float y2 = price_to_y(d.p2);
            float dx = x2 - x1;
            float dy = y2 - y1;
            if (std::abs(dx) > 0.5f) {
                // Slope in y/x
                float slope = dy / dx;
                // Find y at chart left (chart_x) and chart right (chart_x + chart_w)
                float y_left  = y1 + slope * (chart_x - x1);
                float y_right = y1 + slope * (chart_x + chart_w - x1);
                // Clip to chart vertical bounds [chart_y, chart_y+price_h]
                float lx = chart_x,         ly = y_left;
                float rx = chart_x + chart_w, ry_val = y_right;
                // Only draw if the line passes through the visible area
                float ymin = std::min(ly, ry_val);
                float ymax = std::max(ly, ry_val);
                if (ymax >= chart_y && ymin <= chart_y + price_h) {
                    ui_draw_line(r, lx, ly, rx, ry_val, color);
                }
            } else {
                // Vertical line (degenerate): just draw at x1
                ui_draw_line(r, x1, chart_y, x1, chart_y + price_h, color);
            }
            // Draw endpoints markers
            ui_fill_rect(r, x1 - 2.f, y1 - 2.f, 5.f, 5.f, color);
            ui_fill_rect(r, x2 - 2.f, y2 - 2.f, 5.f, 5.f, color);
        } else if (d.tool == AppState::Tool::TEXT_NOTE) {
            float x = time_to_x(d.t1);
            float y = price_to_y(d.p1);
            int tw = ui_text_width(state->font_sm, d.text.c_str());
            ui_fill_rect(r, x, y - 20.f, tw + 8.f, 20.f, tc.bg_header);
            ui_draw_rect(r, x, y - 20.f, tw + 8.f, 20.f, color);
            ui_draw_text(r, state->font_sm, x + 4.f, y - 18.f, d.text.c_str(), tc.text_primary);
        }
    };

    for (const auto& d : state->drawings) {
        draw_single(d, tc.accent);
    }
    if (state->drawing_in_progress) {
        draw_single(state->current_drawing, tc.text_secondary);
    }

    // Crosshair and Hover info
    bool hovered = point_in(state->mouse_x, state->mouse_y, chart_x, chart_y, chart_w, chart_h);
    if (hovered) {
        float cursor_price = min_p + range * (1.0f - (state->mouse_y - chart_y) / price_h);
        state->hover_price = cursor_price;
        
        float rel_x = state->mouse_x - chart_x;
        int hover_idx = start + (int)(rel_x / candle_w);
        if (hover_idx >= start && hover_idx < end) {
            state->hover_time = state->candles[hover_idx].open_time;
        }

        // Draw crosshair if selected or hovering
        if (state->active_tool == AppState::Tool::CROSSHAIR || hovered) {
            ui_draw_line(r, chart_x, state->mouse_y, chart_x + chart_w, state->mouse_y, tc.text_muted); // Horizontal
            ui_draw_line(r, state->mouse_x, chart_y, state->mouse_x, chart_y + chart_h, tc.text_muted); // Vertical
        }

        std::string cp_str = fmt_price(cursor_price);
        float ptag_w = (float)(ui_text_width(state->font_sm, cp_str.c_str()) + 10);
        ui_fill_rect(r, chart_x + chart_w + 2.f, state->mouse_y - 9.f, ptag_w, 18.f, tc.text_muted);
        ui_draw_text(r, state->font_sm, chart_x + chart_w + 5.f, state->mouse_y - 7.f, cp_str.c_str(), tc.bg_window);
        
        if (hover_idx >= start && hover_idx < end) {
            const auto& hc = state->candles[hover_idx];
            
            // Draw time tag on X axis
            bool date_only = (state->tf_index >= 7);
            std::string ct_str = fmt_time(hc.open_time, 2); 
            float time_tag_w = (float)(ui_text_width(state->font_sm, ct_str.c_str()) + 10);
            
            float hover_tag_x = state->mouse_x - time_tag_w * 0.5f;
            hover_tag_x = std::max(chart_x, std::min(hover_tag_x, chart_x + chart_w - time_tag_w));

            ui_fill_rect(r, hover_tag_x, chart_y + chart_h + 2.f, time_tag_w, 18.f, tc.text_muted);
            ui_draw_text(r, state->font_sm, hover_tag_x + 5.f, chart_y + chart_h + 4.f, ct_str.c_str(), tc.bg_window);

            // Draw OHLCV text at top
            float chg_pct = (hc.close - hc.open) / hc.open * 100.f;
            char ohlcv_buf[256];
            snprintf(ohlcv_buf, sizeof(ohlcv_buf), "O: %s  H: %s  L: %s  C: %s  Vol: %s  Chg: %s",
                     fmt_price(hc.open).c_str(), fmt_price(hc.high).c_str(),
                     fmt_price(hc.low).c_str(), fmt_price(hc.close).c_str(),
                     fmt_vol(hc.volume).c_str(), fmt_pct(chg_pct).c_str());
            
            ui_draw_text(r, state->font_sm, chart_x + 5.f, chart_y + 5.f, ohlcv_buf, tc.text_primary);
        }
    }
}
