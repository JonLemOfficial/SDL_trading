#pragma once
#include <SDL3/SDL.h>
#include "core/types.hpp"
#include <string>

// =============================================================================
//  Trading Platform — Theme System
// =============================================================================

struct ThemeColors {
    // Backgrounds
    SDL_Color bg_window;
    SDL_Color bg_panel;
    SDL_Color bg_header;
    SDL_Color bg_row_even;
    SDL_Color bg_row_odd;
    SDL_Color bg_row_hover;
    SDL_Color bg_row_sel;
    SDL_Color bg_tab_active;
    SDL_Color bg_tab_idle;
    SDL_Color bg_btn_active;
    SDL_Color bg_btn_idle;
    SDL_Color bg_input;
    SDL_Color bg_input_focused;
    // Borders
    SDL_Color border;
    SDL_Color border_accent;
    // Text
    SDL_Color text_primary;
    SDL_Color text_secondary;
    SDL_Color text_muted;
    SDL_Color text_header;
    SDL_Color text_input;
    // Chart / market colours
    SDL_Color bull;
    SDL_Color bear;
    SDL_Color accent;
    SDL_Color grid;
    SDL_Color price_tag_bg;
    SDL_Color price_tag_text;
    // Misc
    SDL_Color scrollbar_track;
    SDL_Color scrollbar_thumb;
    SDL_Color alert_trigger;
    SDL_Color notification_bg;
};

ThemeColors dark_theme();
ThemeColors light_theme();
ThemeColors get_theme(Theme t);

// Returns "☀" / "☾" / "⚙" label for cycle button
const char* theme_label(Theme t);
// Cycle to next theme mode
Theme       theme_next(Theme t);
