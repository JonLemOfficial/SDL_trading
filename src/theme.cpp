#include "theme.hpp"
#include <cstdlib>
#include <string>

// =============================================================================
//  Dark theme
// =============================================================================
ThemeColors dark_theme() {
    ThemeColors c;
    c.bg_window         = {8,  10, 16,  255};
    c.bg_panel          = {13, 16, 26,  255};
    c.bg_header         = {18, 22, 38,  255};
    c.bg_row_even       = {12, 15, 24,  255};
    c.bg_row_odd        = {15, 19, 30,  255};
    c.bg_row_hover      = {35, 55, 120, 200};
    c.bg_row_sel        = {28, 50, 110, 255};
    c.bg_tab_active     = {38, 60, 150, 255};
    c.bg_tab_idle       = {18, 22, 40,  255};
    c.bg_btn_active     = {55, 80, 190, 255};
    c.bg_btn_idle       = {22, 28, 55,  255};
    c.bg_input          = {16, 20, 34,  255};
    c.bg_input_focused  = {22, 28, 52,  255};
    c.border            = {40, 48, 75,  255};
    c.border_accent     = {60, 90, 200, 255};
    c.text_primary      = {220,228,248, 255};
    c.text_secondary    = {170,180,210, 255};
    c.text_muted        = {90, 100,140, 255};
    c.text_header       = {200,215,255, 255};
    c.text_input        = {210,220,245, 255};
    c.bull              = {0,  205,110, 255};
    c.bear              = {220,55, 60,  255};
    c.accent            = {200,170,30,  255};
    c.grid              = {25, 30, 50,  255};
    c.price_tag_bg      = {200,170,30,  255};
    c.price_tag_text    = {20, 20, 20,  255};
    c.scrollbar_track   = {18, 22, 40,  255};
    c.scrollbar_thumb   = {70, 100,190, 255};
    c.alert_trigger     = {255,200,50,  255};
    c.notification_bg   = {30, 45, 100, 230};
    return c;
}

// =============================================================================
//  Light theme
// =============================================================================
ThemeColors light_theme() {
    ThemeColors c;
    c.bg_window         = {240,242,248, 255};
    c.bg_panel          = {255,255,255, 255};
    c.bg_header         = {230,234,245, 255};
    c.bg_row_even       = {250,251,255, 255};
    c.bg_row_odd        = {242,244,252, 255};
    c.bg_row_hover      = {210,220,255, 180};
    c.bg_row_sel        = {195,215,255, 255};
    c.bg_tab_active     = {100,140,240, 255};
    c.bg_tab_idle       = {215,220,238, 255};
    c.bg_btn_active     = {90, 130,220, 255};
    c.bg_btn_idle       = {210,215,235, 255};
    c.bg_input          = {240,242,250, 255};
    c.bg_input_focused  = {228,235,255, 255};
    c.border            = {195,200,220, 255};
    c.border_accent     = {90, 130,220, 255};
    c.text_primary      = {20, 25, 45,  255};
    c.text_secondary    = {60, 70, 105, 255};
    c.text_muted        = {130,140,170, 255};
    c.text_header       = {30, 45, 90,  255};
    c.text_input        = {20, 25, 50,  255};
    c.bull              = {0,  160,80,  255};
    c.bear              = {200,40, 50,  255};
    c.accent            = {180,140,10,  255};
    c.grid              = {210,215,230, 255};
    c.price_tag_bg      = {80, 130,210, 255};
    c.price_tag_text    = {255,255,255, 255};
    c.scrollbar_track   = {220,224,238, 255};
    c.scrollbar_thumb   = {140,165,230, 255};
    c.alert_trigger     = {200,130,0,   255};
    c.notification_bg   = {200,215,255, 230};
    return c;
}

// =============================================================================
//  System detection
// =============================================================================
ThemeColors get_theme(Theme t) {
    if (t == Theme::LIGHT) return light_theme();
    if (t == Theme::DARK)  return dark_theme();

    // SYSTEM: try GTK_THEME env var, then COLOR_SCHEME dbus hint
    const char* gtk = getenv("GTK_THEME");
    if (gtk) {
        std::string g(gtk);
        if (g.find("light") != std::string::npos ||
            g.find("Light") != std::string::npos)
            return light_theme();
    }
    const char* cs = getenv("GNOME_DESKTOP_SESSION_ID");
    (void)cs; // unused hint for future expansion
    return dark_theme(); // safe default
}

const char* theme_label(Theme t) {
    switch (t) {
        case Theme::DARK:   return "DARK";
        case Theme::LIGHT:  return "LIGHT";
        case Theme::SYSTEM: return "SYS";
        default:            return "?";
    }
}

Theme theme_next(Theme t) {
    switch (t) {
        case Theme::DARK:   return Theme::LIGHT;
        case Theme::LIGHT:  return Theme::SYSTEM;
        case Theme::SYSTEM: return Theme::DARK;
        default:            return Theme::DARK;
    }
}
