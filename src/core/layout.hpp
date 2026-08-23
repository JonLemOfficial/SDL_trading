#pragma once

#include "core/app_state.hpp"

// =============================================================================
//  Layout — panel geometry from window size
// =============================================================================

/// Recomputes all panel rectangles in `state->layout` from the current window size.
void update_layout(AppState* state);
