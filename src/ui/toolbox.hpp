#pragma once

#include "core/app_state.hpp"

// =============================================================================
//  Toolbox — chart drawing tools strip
// =============================================================================

/// Draws the left-side toolbox with tool icons and Clear All button.
void draw_toolbox(AppState* state);

/// Returns true if (mx, my) hits a toolbox tool button; sets `out_tool` on hit.
bool hit_toolbox_tool(AppState* state, float mx, float my, AppState::Tool& out_tool);

/// Returns true if (mx, my) hits the Clear All button at the bottom of the toolbox.
bool hit_toolbox_clear(AppState* state, float mx, float my);
