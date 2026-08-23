#include "modules/market_table.hpp"
#include "services/network.hpp"
#include "ui/ui_utils.hpp"
#include "services/favorites.hpp"
#include "core/constants.hpp"

#include <nlohmann/json.hpp>
#include <algorithm>
#include <thread>
#include <chrono>
#include <cctype>

using json = nlohmann::json;

// =============================================================================
//  Layout constants (table panel)
// =============================================================================


static constexpr float SEARCH_H   = 28.f;
static constexpr float TAB_H      = 26.f;
static constexpr float COL_HDR_H  = 22.f;
static constexpr float ROW_H      = 20.f;
static constexpr float HEADER_H   = 36.f;

// Column layout (offsets from TX_BASE + 4)
struct ColDef { const char* label; float x_off; float width; SortColumn sc; };
static float cs_base(AppState* state) { return state->layout.table_x + 4.f; }

static ColDef cols_def[6] = {
    {"SYMBOL", 0.f,     78.f,  SortColumn::NAME},
    {"PRICE",  82.f,    90.f,  SortColumn::PRICE},
    {"24H%",   176.f,   65.f,  SortColumn::CHANGE_24H},
    {"VOL",    245.f,   65.f,  SortColumn::VOLUME_24H},
    {"HIGH",   314.f,   65.f,  SortColumn::HIGH_24H},
    {"LOW",    383.f,   60.f,  SortColumn::LOW_24H},
};

// =============================================================================
//  Helpers
// =============================================================================

static bool str_contains_ci(const std::string& hay, const std::string& needle) {
    if (needle.empty()) return true;
    auto it = std::search(hay.begin(), hay.end(),
                          needle.begin(), needle.end(),
                          [](char a, char b){ return std::toupper(a) == std::toupper(b); });
    return it != hay.end();
}

static void sort_pairs(std::vector<PairInfo>& pairs, SortColumn col, SortDir dir) {
    std::sort(pairs.begin(), pairs.end(), [&](const PairInfo& a, const PairInfo& b) {
        bool asc = (dir == SortDir::ASC);
        switch (col) {
            case SortColumn::NAME:       return asc ? a.symbol < b.symbol : a.symbol > b.symbol;
            case SortColumn::PRICE:      return asc ? a.price < b.price : a.price > b.price;
            case SortColumn::CHANGE_24H: return asc ? a.change_24h < b.change_24h : a.change_24h > b.change_24h;
            case SortColumn::VOLUME_24H: return asc ? a.volume_24h < b.volume_24h : a.volume_24h > b.volume_24h;
            case SortColumn::HIGH_24H:   return asc ? a.high_24h < b.high_24h : a.high_24h > b.high_24h;
            case SortColumn::LOW_24H:    return asc ? a.low_24h < b.low_24h : a.low_24h > b.low_24h;
            default: return false;
        }
    });
}

static float row_y_start(AppState* state) {
    float TY_BASE = state->layout.table_y;
    float header_bottom = TY_BASE + HEADER_H;
    float search_bottom = header_bottom + SEARCH_H + 2.f;
    float tab_bottom    = search_bottom + TAB_H + 2.f;
    float col_bottom    = tab_bottom + COL_HDR_H + 2.f;
    return col_bottom;
}

static float visible_rows_h(AppState* state) {
    float TY_BASE = state->layout.table_y;
    float TH = state->layout.table_h;
    return TH - (row_y_start(state) - TY_BASE) - 4.f;
}

static int max_rows_count(AppState* state) {
    return (int)(visible_rows_h(state) / ROW_H);
}

// =============================================================================
//  Pair fetch thread
// =============================================================================

void fetch_pairs(AppState* state) {
    while (state->running) {
        std::string spot_raw = http_get("https://api.binance.com/api/v3/ticker/24hr");
        std::string fut_raw  = http_get("https://fapi.binance.com/fapi/v1/ticker/24hr");

        auto parse_list = [](const std::string& raw, bool is_fut) -> std::vector<PairInfo> {
            std::vector<PairInfo> list;
            if (raw.empty()) return list;
            try {
                json j = json::parse(raw);
                for (auto& item : j) {
                    std::string sym = item["symbol"].get<std::string>();
                    if (sym.size() > 4 && sym.substr(sym.size() - 4) == "USDT") {
                        PairInfo p;
                        p.symbol      = sym;
                        p.base_asset  = sym.substr(0, sym.size() - 4);
                        p.quote_asset = "USDT";
                        p.price       = std::stof(item["lastPrice"].get<std::string>());
                        p.change_24h  = std::stof(item["priceChangePercent"].get<std::string>());
                        p.volume_24h  = std::stof(item["quoteVolume"].get<std::string>());
                        p.high_24h    = std::stof(item["highPrice"].get<std::string>());
                        p.low_24h     = std::stof(item["lowPrice"].get<std::string>());
                        p.is_futures  = is_fut;
                        list.push_back(p);
                    }
                }
            } catch (...) {}
            return list;
        };

        auto spot_list = parse_list(spot_raw, false);
        auto fut_list  = parse_list(fut_raw,  true);

        {
            std::lock_guard<std::mutex> lock(state->pairs_mtx);
            if (!spot_list.empty()) state->pairs_spot    = std::move(spot_list);
            if (!fut_list.empty())  state->pairs_futures = std::move(fut_list);
            state->pairs_dirty = true;
        }

        for (int i = 0; i < 100 && state->running; i++)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// =============================================================================
//  Sorted/filtered pair list
// =============================================================================

std::vector<PairInfo> get_sorted_filtered_pairs(AppState* state) {
    std::vector<PairInfo> src;
    {
        std::lock_guard<std::mutex> lock(state->pairs_mtx);
        if (state->active_tab == MarketTab::SPOT)
            src = state->pairs_spot;
        else if (state->active_tab == MarketTab::FUTURES)
            src = state->pairs_futures;
        else {
            // FAVORITES: combine spot & futures, deduplicate
            for (auto& p : state->pairs_spot)
                if (fav_has(state->favorites, p.symbol)) src.push_back(p);
        }
    }
    // Search filter
    if (!state->search.query.empty()) {
        std::vector<PairInfo> filtered;
        for (auto& p : src)
            if (str_contains_ci(p.symbol, state->search.query) ||
                str_contains_ci(p.base_asset, state->search.query))
                filtered.push_back(p);
        src = std::move(filtered);
    }
    sort_pairs(src, state->sort_col, state->sort_dir);
    return src;
}

// =============================================================================
//  Hit-test helpers
// =============================================================================

bool hit_theme_button(AppState* state, float mx, float my) {
    float TX_BASE = state->layout.table_x, TY_BASE = state->layout.table_y, TW = state->layout.table_w, TH = state->layout.table_h;
    // Top-right corner of table header
    float bx = TX_BASE + TW - 52.f;
    float by = TY_BASE + 4.f;
    return point_in(mx, my, bx, by, 48.f, 26.f);
}

bool hit_search_bar(AppState* state, float mx, float my) {
    float TX_BASE = state->layout.table_x, TY_BASE = state->layout.table_y, TW = state->layout.table_w, TH = state->layout.table_h;
    float sy = TY_BASE + HEADER_H + 2.f;
    return point_in(mx, my, TX_BASE + 2.f, sy, TW - 4.f, SEARCH_H);
}

int hit_tab(AppState* state, float mx, float my) {
    float TX_BASE = state->layout.table_x, TY_BASE = state->layout.table_y, TW = state->layout.table_w, TH = state->layout.table_h;
    float tab_y = TY_BASE + HEADER_H + SEARCH_H + 4.f;
    float tab_w = TW / 3.f;
    for (int i = 0; i < 3; i++) {
        float tbx = TX_BASE + i * tab_w;
        if (point_in(mx, my, tbx, tab_y, tab_w, TAB_H)) return i;
    }
    return -1;
}

int hit_col_header(AppState* state, float mx, float my) {
    float TX_BASE = state->layout.table_x, TY_BASE = state->layout.table_y, TW = state->layout.table_w, TH = state->layout.table_h;
    float tab_y   = TY_BASE + HEADER_H + SEARCH_H + 4.f;
    float col_y   = tab_y + TAB_H + 2.f;
    if (my < col_y || my > col_y + COL_HDR_H) return -1;
    float cs = cs_base(state);
    for (int i = 0; i < 6; i++) {
        float cx = cs + cols_def[i].x_off;
        if (point_in(mx, my, cx, col_y, cols_def[i].width, COL_HDR_H)) return i;
    }
    return -1;
}

int hit_table_row(float mx, float my, AppState* state,
                  const std::vector<PairInfo>& sorted) {
    float TX_BASE = state->layout.table_x;
    float TW = state->layout.table_w;
    float rys = row_y_start(state);
    float vh  = visible_rows_h(state);
    if (!point_in(mx, my, TX_BASE, rys, TW, vh)) return -1;
    int idx = (int)((my - rys) / ROW_H);
    if (idx < 0 || idx >= max_rows_count(state)) return -1;
    int absolute = state->table_scroll + idx;
    if (absolute >= (int)sorted.size()) return -1;
    return absolute;
}

bool hit_fav_star(AppState* state, float mx, float my, int row_idx) {
    float TX_BASE = state->layout.table_x, TY_BASE = state->layout.table_y, TW = state->layout.table_w, TH = state->layout.table_h;
    float rys = row_y_start(state);
    float ry  = rys + row_idx * ROW_H;
    float sx  = TX_BASE + TW - 18.f;
    return point_in(mx, my, sx, ry, 14.f, ROW_H);
}

// =============================================================================
//  Table rendering
// =============================================================================

void draw_table(AppState* state) {
    float TX_BASE = state->layout.table_x, TY_BASE = state->layout.table_y, TW = state->layout.table_w, TH = state->layout.table_h;
    // Dynamically adjust columns width based on TW
    cols_def[0].width = TW * 0.15f;
    cols_def[1].width = TW * 0.17f;
    cols_def[2].width = TW * 0.17f;
    cols_def[3].width = TW * 0.17f;
    cols_def[4].width = TW * 0.17f;
    cols_def[5].width = TW * 0.17f;
    cols_def[1].x_off = cols_def[0].width + 10.f;
    cols_def[2].x_off = cols_def[1].x_off + cols_def[1].width;
    cols_def[3].x_off = cols_def[2].x_off + cols_def[2].width;
    cols_def[4].x_off = cols_def[3].x_off + cols_def[3].width;
    cols_def[5].x_off = cols_def[4].x_off + cols_def[4].width;

    SDL_Renderer* r = state->renderer;
    const ThemeColors& tc = state->theme_colors;

    float tx = TX_BASE, ty = TY_BASE, tw = TW, th = TH;

    ui_fill_rect(r, tx, ty, tw, th, tc.bg_panel);
    ui_draw_rect(r, tx, ty, tw, th, tc.border);

    // ── Header bar ───────────────────────────────────────────────────────────
    ui_fill_rect(r, tx, ty, tw, HEADER_H, tc.bg_header);
    ui_draw_text(r, state->font_lg, tx + 10.f, ty + 8.f, "BINANCE MARKETS", tc.text_header);

    // Theme toggle button (top-right)
    float tbx = tx + tw - 52.f;
    float tby = ty + 4.f;
    ui_fill_rect(r, tbx, tby, 48.f, 26.f, tc.bg_btn_idle);
    ui_draw_rect(r, tbx, tby, 48.f, 26.f, tc.border_accent);
    ui_draw_text_centered(r, state->font_sm, tbx, tby, 48.f, 26.f,
                          theme_label(state->theme), tc.text_secondary);

    // ── Search bar ───────────────────────────────────────────────────────────
    float sy = ty + HEADER_H + 2.f;
    ui_search_bar(r, state->font_sm,
                  tx + 2.f, sy, tw - 4.f, SEARCH_H,
                  state->search.query, state->search.focused, tc);

    // ── Tabs: SPOT | FUTURES | FAV ────────────────────────────────────────────
    float tab_y = sy + SEARCH_H + 2.f;
    float tab_w = tw / 3.f;
    const char* tab_labels[] = {"SPOT", "FUTURES", "FAV"};
    for (int i = 0; i < 3; i++) {
        bool active = ((i == 0 && state->active_tab == MarketTab::SPOT) ||
                       (i == 1 && state->active_tab == MarketTab::FUTURES) ||
                       (i == 2 && state->active_tab == MarketTab::FAVORITES));
        float tbx2 = tx + i * tab_w;
        SDL_Color bg  = active ? tc.bg_tab_active : tc.bg_tab_idle;
        SDL_Color txt = active ? tc.text_primary   : tc.text_muted;
        ui_fill_rect(r, tbx2, tab_y, tab_w, TAB_H, bg);
        ui_draw_rect(r, tbx2, tab_y, tab_w, TAB_H, tc.border);
        ui_draw_text_centered(r, state->font_sm, tbx2, tab_y, tab_w, TAB_H,
                              tab_labels[i], txt);
    }

    // ── Column headers ────────────────────────────────────────────────────────
    float col_y = tab_y + TAB_H + 2.f;
    ui_fill_rect(r, tx, col_y, tw, COL_HDR_H, tc.bg_header);
    float cs = cs_base(state);
    for (int i = 0; i < 6; i++) {
        bool active_sort = (cols_def[i].sc == state->sort_col);
        SDL_Color clc = active_sort ? tc.text_primary : tc.text_muted;
        std::string lbl = cols_def[i].label;
        if (active_sort) lbl += (state->sort_dir == SortDir::ASC ? " ^" : " v");
        ui_draw_text(r, state->font_sm, cs + cols_def[i].x_off, col_y + 4.f,
                     lbl.c_str(), clc);
    }
    // ★ column header
    ui_draw_text(r, state->font_sm, tx + tw - 18.f, col_y + 4.f, "★", tc.text_muted);

    // ── Rows ─────────────────────────────────────────────────────────────────
    float rys = row_y_start(state);
    float vh  = visible_rows_h(state);
    int   max_r = max_rows_count(state);

    auto sorted = get_sorted_filtered_pairs(state);
    int total_rows = (int)sorted.size();
    state->table_scroll = std::max(0, std::min(state->table_scroll, total_rows - max_r));
    int start_row = state->table_scroll;
    int end_row   = std::min(total_rows, start_row + max_r);

    if (sorted.empty()) {
        ui_draw_text_centered(r, state->font, tx, rys, tw, 60.f,
                              "Loading pairs...", tc.text_muted);
    }

    for (int i = start_row; i < end_row; i++) {
        const auto& p  = sorted[i];
        int   row_idx  = i - start_row;
        float ry       = rys + row_idx * ROW_H;

        // Row background
        SDL_Color row_bg;
        if (i == state->table_hover)
            row_bg = tc.bg_row_hover;
        else if (p.symbol == state->chart_symbol)
            row_bg = tc.bg_row_sel;
        else
            row_bg = (row_idx % 2 == 0) ? tc.bg_row_even : tc.bg_row_odd;
        ui_fill_rect(r, tx + 1.f, ry, tw - 2.f, ROW_H, row_bg);

        // Text
        bool pos = p.change_24h >= 0.f;
        SDL_Color chg_col = pos ? tc.bull : tc.bear;

        ui_draw_text(r, state->font_sm, cs + cols_def[0].x_off, ry + 3.f,
                     p.base_asset.c_str(), tc.text_primary);
        ui_draw_text(r, state->font_sm, cs + cols_def[1].x_off, ry + 3.f,
                     fmt_price(p.price).c_str(), tc.text_secondary);
        ui_draw_text(r, state->font_sm, cs + cols_def[2].x_off, ry + 3.f,
                     fmt_pct(p.change_24h).c_str(), chg_col);
        ui_draw_text(r, state->font_sm, cs + cols_def[3].x_off, ry + 3.f,
                     fmt_vol(p.volume_24h).c_str(), tc.text_secondary);
        ui_draw_text(r, state->font_sm, cs + cols_def[4].x_off, ry + 3.f,
                     fmt_price(p.high_24h).c_str(), tc.bull);
        ui_draw_text(r, state->font_sm, cs + cols_def[5].x_off, ry + 3.f,
                     fmt_price(p.low_24h).c_str(), tc.bear);

        // Favourite star
        bool is_fav = fav_has(state->favorites, p.symbol);
        SDL_Color star_col = is_fav ? tc.accent : tc.text_muted;
        ui_draw_text(r, state->font_sm, tx + tw - 18.f, ry + 3.f,
                     is_fav ? "★" : "☆", star_col);
    }

    // ── Scrollbar ─────────────────────────────────────────────────────────────
    if (total_rows > max_r) {
        float sb_x    = tx + tw - 6.f;
        float sb_y    = rys;
        float sb_h    = vh;
        ui_fill_rect(r, sb_x, sb_y, 4.f, sb_h, tc.scrollbar_track);
        float thumb_h = std::max(20.f, sb_h * max_r / total_rows);
        float thumb_y = sb_y + (sb_h - thumb_h) *
                        (float)start_row / std::max(1, total_rows - max_r);
        ui_fill_rect(r, sb_x, thumb_y, 4.f, thumb_h, tc.scrollbar_thumb);
    }

    // Pair count (bottom-right of column header)
    std::string cnt_str = std::to_string(total_rows) + " pairs";
    ui_draw_text_right(r, state->font_sm, tx + tw - 80.f, col_y + 4.f, 72.f,
                       cnt_str.c_str(), tc.text_muted);
}
