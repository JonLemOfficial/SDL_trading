#pragma once

#include "core/app_state.hpp"

// =============================================================================
//  Chrome — shared UI overlays (dividers, notifications, placeholders)
// =============================================================================

/// Draws the vertical and horizontal panel resize dividers.
void draw_dividers(AppState* state);

/// Draws the auto-fading notification banner (if active).
void draw_notification(AppState* state);

/// Draws a centered placeholder label for tabs not yet implemented.
void draw_placeholder(AppState* state, const char* label);
