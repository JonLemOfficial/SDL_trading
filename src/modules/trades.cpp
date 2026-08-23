#include "modules/trades.hpp"
#include "services/network.hpp"
#include "ui/ui_utils.hpp"

#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <algorithm>

using json = nlohmann::json;

// =============================================================================
//  Fetch thread
// =============================================================================

void fetch_trades(AppState* state) {
    while (state->running) {
        std::string symbol = state->chart_symbol;
        std::string url = "https://api.binance.com/api/v3/trades?symbol=" +
                          symbol + "&limit=50";
        std::string raw = http_get(url);

        if (!raw.empty()) {
            try {
                json j = json::parse(raw);
                std::vector<Trade> list;
                list.reserve(j.size());
                for (auto& item : j) {
                    Trade t;
                    t.id           = item["id"].get<int64_t>();
                    t.price        = std::stof(item["price"].get<std::string>());
                    t.qty          = std::stof(item["qty"].get<std::string>());
                    t.time_ms      = item["time"].get<int64_t>();
                    t.buyer_maker  = item["isBuyerMaker"].get<bool>();
                    list.push_back(t);
                }
                // Newest first
                std::reverse(list.begin(), list.end());
                std::lock_guard<std::mutex> lock(state->trades_mtx);
                state->trades       = std::move(list);
                state->trades_dirty = true;
            } catch (...) {}
        }

        for (int i = 0; i < 20 && state->running; i++)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// =============================================================================
//  Trades rendering
// =============================================================================

void draw_trades(AppState* state, float x, float y, float w, float h) {
    SDL_Renderer* r = state->renderer;
    const ThemeColors& tc = state->theme_colors;

    ui_fill_rect(r, x, y, w, h, tc.bg_panel);

    // Column header
    float hdr_y = y + 2.f;
    ui_draw_text(r, state->font_sm, x + 4.f,     hdr_y, "PRICE",    tc.text_muted);
    ui_draw_text(r, state->font_sm, x + w*0.4f,  hdr_y, "QTY",      tc.text_muted);
    ui_draw_text(r, state->font_sm, x + w*0.72f, hdr_y, "TIME",     tc.text_muted);
    ui_draw_text(r, state->font_sm, x + w - 20.f,hdr_y, "S",        tc.text_muted);

    std::lock_guard<std::mutex> lock(state->trades_mtx);
    if (state->trades.empty()) {
        ui_draw_text_centered(r, state->font_sm, x, y, w, h,
                              "No trade data", tc.text_muted);
        return;
    }

    float row_h = 16.f;
    float cur_y = hdr_y + 14.f;
    float max_y = y + h - 4.f;
    int max_rows = (int)((max_y - cur_y) / row_h);

    int scroll = std::max(0, std::min(state->trades_scroll,
                          (int)state->trades.size() - max_rows));

    for (int i = scroll; i < (int)state->trades.size(); i++) {
        if (cur_y + row_h > max_y) break;
        const auto& t = state->trades[i];
        bool is_sell = t.buyer_maker; // maker is buyer → this trade was a sell
        SDL_Color price_col = is_sell ? tc.bear : tc.bull;
        SDL_Color side_col  = is_sell ? tc.bear : tc.bull;

        ui_draw_text(r, state->font_sm, x + 4.f,     cur_y, fmt_price(t.price).c_str(), price_col);
        ui_draw_text(r, state->font_sm, x + w*0.4f,  cur_y, fmt_vol(t.qty).c_str(),     tc.text_secondary);
        ui_draw_text(r, state->font_sm, x + w*0.72f, cur_y, fmt_time(t.time_ms).c_str(),tc.text_muted);
        ui_draw_text(r, state->font_sm, x + w - 18.f,cur_y, is_sell ? "S" : "B",       side_col);

        cur_y += row_h;
    }
}

