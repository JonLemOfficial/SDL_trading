#pragma once

#include "core/app_state.hpp"

// =============================================================================
//  Titlebar — custom borderless window chrome
// =============================================================================

constexpr float TITLEBAR_BTN_W = 32.f;

/// Returns true if (mx, my) hits the close button.
bool hit_close_btn(AppState* state, float mx, float my);

/// Returns true if (mx, my) hits the maximize/restore button.
bool hit_max_btn(AppState* state, float mx, float my);

/// Returns true if (mx, my) hits the minimize button.
bool hit_min_btn(AppState* state, float mx, float my);

/// Draws the custom titlebar: live stats + window control buttons.
void draw_titlebar(AppState* state);
