#include "modules/orderbook.hpp"
#include "services/network.hpp"
#include "ui/ui_utils.hpp"

#include <nlohmann/json.hpp>
#include <algorithm>
#include <thread>
#include <chrono>

using json = nlohmann::json;

// =============================================================================
//  Fetch thread
// =============================================================================

void fetch_orderbook(AppState* state) {
    while (state->running) {
        std::string symbol = state->chart_symbol;
        std::string url = "https://api.binance.com/api/v3/depth?symbol=" +
                          symbol + "&limit=20";
        std::string raw = http_get(url);

        if (!raw.empty()) {
            try {
                json j = json::parse(raw);
                std::vector<OBEntry> bids, asks;
                for (auto& b : j["bids"]) {
                    OBEntry e;
                    e.price = std::stof(b[0].get<std::string>());
                    e.qty   = std::stof(b[1].get<std::string>());
                    bids.push_back(e);
                }
                for (auto& a : j["asks"]) {
                    OBEntry e;
                    e.price = std::stof(a[0].get<std::string>());
                    e.qty   = std::stof(a[1].get<std::string>());
                    asks.push_back(e);
                }
                std::lock_guard<std::mutex> lock(state->orderbook.mtx);
                state->orderbook.bids = std::move(bids);
                state->orderbook.asks = std::move(asks);
                state->ob_dirty       = true;
            } catch (...) {}
        }

        for (int i = 0; i < 20 && state->running; i++)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// =============================================================================
//  Order book rendering
// =============================================================================

void draw_orderbook(AppState* state, float x, float y, float w, float h) {
    SDL_Renderer* r = state->renderer;
    const ThemeColors& tc = state->theme_colors;

    ui_fill_rect(r, x, y, w, h, tc.bg_panel);

    std::lock_guard<std::mutex> lock(state->orderbook.mtx);
    auto& bids = state->orderbook.bids;
    auto& asks = state->orderbook.asks;

    if (bids.empty() && asks.empty()) {
        ui_draw_text_centered(r, state->font_sm, x, y, w, h,
                              "No order book data", tc.text_muted);
        return;
    }

    // Max quantity for bar scaling
    float max_qty = 1.f;
    for (auto& e : bids) max_qty = std::max(max_qty, e.qty);
    for (auto& e : asks) max_qty = std::max(max_qty, e.qty);

    // Column header
    float hdr_y = y + 2.f;
    float row_h = (h - 26.f) / (float)(bids.size() + asks.size() + 1);
    row_h = std::max(14.f, std::min(row_h, 20.f));

    ui_draw_text(r, state->font_sm, x + 4.f, hdr_y, "PRICE", tc.text_muted);
    ui_draw_text_right(r, state->font_sm, x + 4.f, hdr_y, w - 8.f, "QTY", tc.text_muted);

    float cur_y = hdr_y + 14.f;

    // Asks (sell side — upper half, reversed so best ask is closest to spread)
    int max_show = (int)((h - 30.f) / row_h / 2.f);
    max_show = std::min(max_show, (int)asks.size());

    // Apply scroll offset (clamp so we don't scroll past available data)
    int scroll = std::max(0, std::min(state->ob_scroll,
                          (int)asks.size() - max_show));

    // Draw asks from worst to best (top to bottom), accounting for scroll
    for (int i = max_show - 1 + scroll; i >= scroll; i--) {
        if (i < 0 || i >= (int)asks.size()) continue;
        const auto& e = asks[i];
        // Bar
        float bar_w = (e.qty / max_qty) * (w - 4.f);
        SDL_Color bar_col = tc.bear;
        bar_col.a = 60;
        ui_fill_rect(r, x + w - 2.f - bar_w, cur_y, bar_w, row_h - 1.f, bar_col);
        // Text
        ui_draw_text(r, state->font_sm, x + 4.f, cur_y + 1.f,
                     fmt_price(e.price).c_str(), tc.bear);
        std::string qty_s = fmt_vol(e.qty);
        ui_draw_text_right(r, state->font_sm, x + 4.f, cur_y + 1.f, w - 8.f,
                           qty_s.c_str(), tc.text_secondary);
        cur_y += row_h;
    }

    // Spread line
    if (!bids.empty() && !asks.empty()) {
        float spread = asks.front().price - bids.front().price;
        std::string spread_str = "Spread: " + fmt_price(spread);
        ui_fill_rect(r, x, cur_y, w, row_h, tc.bg_header);
        ui_draw_text_centered(r, state->font_sm, x, cur_y, w, row_h,
                              spread_str.c_str(), tc.accent);
        cur_y += row_h;
    }

    // Bids (buy side)
    int bid_show = std::min(max_show, (int)bids.size());
    for (int i = 0; i < bid_show; i++) {
        const auto& e = bids[i];
        float bar_w = (e.qty / max_qty) * (w - 4.f);
        SDL_Color bar_col = tc.bull;
        bar_col.a = 60;
        ui_fill_rect(r, x + 2.f, cur_y, bar_w, row_h - 1.f, bar_col);
        ui_draw_text(r, state->font_sm, x + 4.f, cur_y + 1.f,
                     fmt_price(e.price).c_str(), tc.bull);
        std::string qty_s = fmt_vol(e.qty);
        ui_draw_text_right(r, state->font_sm, x + 4.f, cur_y + 1.f, w - 8.f,
                           qty_s.c_str(), tc.text_secondary);
        cur_y += row_h;
    }
}

