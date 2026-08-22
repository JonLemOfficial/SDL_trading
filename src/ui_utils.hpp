#pragma once
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include "theme.hpp"

// =============================================================================
//  Trading Platform — UI Utilities / Draw Helpers
// =============================================================================

// ── Primitives ───────────────────────────────────────────────────────────────

void ui_fill_rect (SDL_Renderer*, float x, float y, float w, float h, SDL_Color);
void ui_fill_rect_rounded(SDL_Renderer*, float x, float y, float w, float h,
                          float radius, SDL_Color);
void ui_draw_rect (SDL_Renderer*, float x, float y, float w, float h, SDL_Color);
void ui_draw_line (SDL_Renderer*, float x1, float y1, float x2, float y2, SDL_Color);

// ── Text ─────────────────────────────────────────────────────────────────────

void ui_draw_text         (SDL_Renderer*, TTF_Font*, float x, float y,
                           const char*, SDL_Color);
void ui_draw_text_centered(SDL_Renderer*, TTF_Font*, float x, float y,
                           float w, float h, const char*, SDL_Color);
void ui_draw_text_right   (SDL_Renderer*, TTF_Font*, float x, float y,
                           float w, const char*, SDL_Color);
int  ui_text_width        (TTF_Font*, const char*);
int  ui_text_height       (TTF_Font*, const char*);

// ── Hit-testing ──────────────────────────────────────────────────────────────

bool point_in(float px, float py, float rx, float ry, float rw, float rh);

// ── Number / time formatters ─────────────────────────────────────────────────

std::string fmt_price(float p);
std::string fmt_pct  (float p);
std::string fmt_vol  (float v);
std::string fmt_time (int64_t ms, int format = 0); // 0=Time, 1=Date, 2=Both

// ── Simple button helper ─────────────────────────────────────────────────────
// Returns true if the button area was clicked (caller must pass click position).
// Draws the button and returns whether (mx,my) is inside it.
bool ui_button(SDL_Renderer* r, TTF_Font* f,
               float x, float y, float w, float h,
               const char* label,
               SDL_Color bg, SDL_Color text_col,
               SDL_Color border_col,
               float mx, float my, bool clicked);

// ── Search bar ───────────────────────────────────────────────────────────────
// Draws a text-input bar; returns true if it was clicked (focus request).
bool ui_search_bar(SDL_Renderer* r, TTF_Font* f,
                   float x, float y, float w, float h,
                   const std::string& query, bool focused,
                   const ThemeColors& tc);
