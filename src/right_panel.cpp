#include "right_panel.hpp"
#include "orderbook.hpp"
#include "trades.hpp"
#include "alerts.hpp"
#include "ui_utils.hpp"
#include "constants.hpp"

static constexpr float TAB_H   = 26.f;
static constexpr float HDR_H   = 28.f;

void right_panel_content_rect(AppState* state, float& x, float& y, float& w, float& h) {
    x = state->layout.right_tabs_x;
    y = state->layout.right_tabs_y + HDR_H + TAB_H;
    w = state->layout.right_tabs_w;
    h = state->layout.right_tabs_h - HDR_H - TAB_H;
}

int hit_right_tab(AppState* state, float mx, float my) {
    float tab_y = state->layout.right_tabs_y + HDR_H;
    float tab_w = state->layout.right_tabs_w / 3.f;
    for (int i = 0; i < 3; i++) {
        float tbx = state->layout.right_tabs_x + i * tab_w;
        if (point_in(mx, my, tbx, tab_y, tab_w, TAB_H)) return i;
    }
    return -1;
}

void draw_right_panel(AppState* state) {
    SDL_Renderer* r = state->renderer;
    const ThemeColors& tc = state->theme_colors;

    float RP_X = state->layout.right_tabs_x;
    float RP_Y = state->layout.right_tabs_y;
    float RP_W = state->layout.right_tabs_w;
    float RP_H = state->layout.right_tabs_h;

    // Panel background + border
    ui_fill_rect(r, RP_X, RP_Y, RP_W, RP_H, tc.bg_panel);
    ui_draw_rect(r, RP_X, RP_Y, RP_W, RP_H, tc.border);

    // Header: symbol name
    ui_fill_rect(r, RP_X, RP_Y, RP_W, HDR_H, tc.bg_header);
    std::string hdr = state->chart_symbol + "  —  Asset Detail";
    ui_draw_text(r, state->font_sm, RP_X + 8.f, RP_Y + 7.f, hdr.c_str(), tc.text_header);

    // Tabs: ORDER BOOK | TRADES | ALERTS
    const char* tab_names[] = {"ORDER BOOK", "TRADES", "ALERTS"};
    float tab_w = RP_W / 3.f;
    float tab_y = RP_Y + HDR_H;
    for (int i = 0; i < 3; i++) {
        bool active = ((i == 0 && state->right_tab == RightTab::ORDERBOOK) ||
                       (i == 1 && state->right_tab == RightTab::TRADES) ||
                       (i == 2 && state->right_tab == RightTab::ALERTS));
        float tbx = RP_X + i * tab_w;
        SDL_Color bg  = active ? tc.bg_tab_active : tc.bg_tab_idle;
        SDL_Color txt = active ? tc.text_primary   : tc.text_muted;
        ui_fill_rect(r, tbx, tab_y, tab_w, TAB_H, bg);
        ui_draw_rect(r, tbx, tab_y, tab_w, TAB_H, tc.border);
        ui_draw_text_centered(r, state->font_sm, tbx, tab_y, tab_w, TAB_H, tab_names[i], txt);
    }

    // Content area
    float cx, cy, cw, ch;
    right_panel_content_rect(state, cx, cy, cw, ch);
    ui_draw_rect(r, cx, cy, cw, ch, tc.border);

    switch (state->right_tab) {
        case RightTab::ORDERBOOK: draw_orderbook(state, cx, cy, cw, ch); break;
        case RightTab::TRADES:    draw_trades   (state, cx, cy, cw, ch); break;
        case RightTab::ALERTS:    draw_alerts   (state, cx, cy, cw, ch); break;
    }
}
