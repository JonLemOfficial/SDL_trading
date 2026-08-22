#include "ui_utils.hpp"
#include <cstring>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cmath>
#include <algorithm>

// =============================================================================
//  Primitives
// =============================================================================

void ui_fill_rect(SDL_Renderer* r, float x, float y, float w, float h, SDL_Color c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_FRect rc = {x, y, w, h};
    SDL_RenderFillRect(r, &rc);
}

// Simple rounded-rect via multiple fill rects (no geometry shader needed)
void ui_fill_rect_rounded(SDL_Renderer* r, float x, float y, float w, float h,
                          float radius, SDL_Color c) {
    if (radius <= 0.f) { ui_fill_rect(r, x, y, w, h, c); return; }
    radius = std::min(radius, std::min(w, h) * 0.5f);
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    // Centre body
    SDL_FRect rc = {x + radius, y, w - radius * 2.f, h};
    SDL_RenderFillRect(r, &rc);
    // Left / right strips
    SDL_FRect rl = {x, y + radius, radius, h - radius * 2.f};
    SDL_RenderFillRect(r, &rl);
    SDL_FRect rr = {x + w - radius, y + radius, radius, h - radius * 2.f};
    SDL_RenderFillRect(r, &rr);
    // Corners — approximate with circles (fan of rects)
    int steps = 6;
    for (int q = 0; q < 4; q++) {
        float cx = (q == 0 || q == 3) ? x + radius : x + w - radius;
        float cy = (q == 0 || q == 1) ? y + radius : y + h - radius;
        for (int i = 0; i < steps; i++) {
            float a0 = (float)M_PI * 0.5f * q + (float)i * (float)M_PI * 0.5f / steps;
            float a1 = a0 + (float)M_PI * 0.5f / steps;
            float x0 = cx + cosf(a0) * radius;
            float y0 = cy + sinf(a0) * radius;
            float x1 = cx + cosf(a1) * radius;
            float y1 = cy + sinf(a1) * radius;
            // small quad between arc segment and centre
            SDL_FRect seg = {std::min(cx, std::min(x0, x1)),
                             std::min(cy, std::min(y0, y1)),
                             std::abs(x1 - x0) + 1.f,
                             std::abs(y1 - y0) + 1.f};
            SDL_RenderFillRect(r, &seg);
        }
    }
}

void ui_draw_rect(SDL_Renderer* r, float x, float y, float w, float h, SDL_Color c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_FRect rc = {x, y, w, h};
    SDL_RenderRect(r, &rc);
}

void ui_draw_line(SDL_Renderer* r, float x1, float y1, float x2, float y2, SDL_Color c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_RenderLine(r, x1, y1, x2, y2);
}

// =============================================================================
//  Text
// =============================================================================

void ui_draw_text(SDL_Renderer* r, TTF_Font* f, float x, float y,
                  const char* txt, SDL_Color c) {
    if (!txt || !txt[0]) return;
    SDL_Surface* s = TTF_RenderText_Blended(f, txt, strlen(txt), c);
    if (!s) return;
    SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
    if (t) {
        SDL_FRect dst = {x, y, (float)s->w, (float)s->h};
        SDL_RenderTexture(r, t, nullptr, &dst);
        SDL_DestroyTexture(t);
    }
    SDL_DestroySurface(s);
}

void ui_draw_text_centered(SDL_Renderer* r, TTF_Font* f,
                            float x, float y, float w, float h,
                            const char* txt, SDL_Color c) {
    if (!txt || !txt[0]) return;
    int tw = 0, th = 0;
    TTF_GetStringSize(f, txt, strlen(txt), &tw, &th);
    ui_draw_text(r, f, x + (w - (float)tw) * 0.5f,
                 y + (h - (float)th) * 0.5f, txt, c);
}

void ui_draw_text_right(SDL_Renderer* r, TTF_Font* f,
                        float x, float y, float w,
                        const char* txt, SDL_Color c) {
    if (!txt || !txt[0]) return;
    int tw = 0, th = 0;
    TTF_GetStringSize(f, txt, strlen(txt), &tw, &th);
    ui_draw_text(r, f, x + w - (float)tw, y, txt, c);
}

int ui_text_width(TTF_Font* f, const char* txt) {
    int w = 0, h = 0;
    TTF_GetStringSize(f, txt, strlen(txt), &w, &h);
    return w;
}

int ui_text_height(TTF_Font* f, const char* txt) {
    int w = 0, h = 0;
    TTF_GetStringSize(f, txt, strlen(txt), &w, &h);
    return h;
}

// =============================================================================
//  Hit-testing
// =============================================================================

bool point_in(float px, float py, float rx, float ry, float rw, float rh) {
    return px >= rx && px <= rx + rw && py >= ry && py <= ry + rh;
}

// =============================================================================
//  Formatters
// =============================================================================

std::string fmt_price(float p) {
    std::ostringstream ss;
    if      (p >= 1000.f) ss << std::fixed << std::setprecision(2) << p;
    else if (p >= 1.f)    ss << std::fixed << std::setprecision(4) << p;
    else                  ss << std::fixed << std::setprecision(6) << p;
    return ss.str();
}

std::string fmt_pct(float p) {
    std::ostringstream ss;
    if (p >= 0.f) ss << "+";
    ss << std::fixed << std::setprecision(2) << p << "%";
    return ss.str();
}

std::string fmt_vol(float v) {
    std::ostringstream ss;
    if      (v >= 1e9f) ss << std::fixed << std::setprecision(2) << v / 1e9f << "B";
    else if (v >= 1e6f) ss << std::fixed << std::setprecision(2) << v / 1e6f << "M";
    else if (v >= 1e3f) ss << std::fixed << std::setprecision(1) << v / 1e3f << "K";
    else                ss << std::fixed << std::setprecision(0) << v;
    return ss.str();
}

std::string fmt_time(int64_t ms, int format) {
    time_t t = (time_t)(ms / 1000);
    struct tm* tm_p = gmtime(&t);
    char buf[64];
    if (format == 1)      strftime(buf, sizeof(buf), "%Y-%m-%d", tm_p);
    else if (format == 2) strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", tm_p);
    else                  strftime(buf, sizeof(buf), "%H:%M:%S", tm_p);
    return buf;
}

// =============================================================================
//  Button helper
// =============================================================================

bool ui_button(SDL_Renderer* r, TTF_Font* f,
               float x, float y, float w, float h,
               const char* label,
               SDL_Color bg, SDL_Color text_col,
               SDL_Color border_col,
               float mx, float my, bool clicked) {
    bool hovered = point_in(mx, my, x, y, w, h);
    // Slightly brighten on hover
    if (hovered) {
        bg.r = (Uint8)std::min(255, (int)bg.r + 20);
        bg.g = (Uint8)std::min(255, (int)bg.g + 20);
        bg.b = (Uint8)std::min(255, (int)bg.b + 20);
    }
    ui_fill_rect(r, x, y, w, h, bg);
    ui_draw_rect(r, x, y, w, h, border_col);
    ui_draw_text_centered(r, f, x, y, w, h, label, text_col);
    return hovered && clicked;
}

// =============================================================================
//  Search bar
// =============================================================================

bool ui_search_bar(SDL_Renderer* r, TTF_Font* f,
                   float x, float y, float w, float h,
                   const std::string& query, bool focused,
                   const ThemeColors& tc) {
    SDL_Color bg = focused ? tc.bg_input_focused : tc.bg_input;
    SDL_Color border = focused ? tc.border_accent : tc.border;
    ui_fill_rect(r, x, y, w, h, bg);
    ui_draw_rect(r, x, y, w, h, border);

    // Placeholder / content
    std::string display = query.empty() ? "Search symbol..." : query;
    SDL_Color text_col  = query.empty() ? tc.text_muted : tc.text_input;
    if (focused && !query.empty()) {
        // Show cursor at end
        display += "|";
    }
    ui_draw_text(r, f, x + 6.f, y + (h - 13.f) * 0.5f, display.c_str(), text_col);
    return false; // caller determines focus from SDL_EVENT_MOUSE_BUTTON_DOWN
}
