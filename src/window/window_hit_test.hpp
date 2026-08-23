#pragma once

#include "core/app_state.hpp"
#include "core/types.hpp"

#include <SDL3/SDL.h>

#include "window/titlebar.hpp"

// =============================================================================
//  Window Hit-Test — native borderless drag/resize zones
// =============================================================================

/// Returns the resize direction for a point near the window border (or NONE).
ResizeDir get_resize_dir(AppState* state, float mx, float my);

/// Maps a resize direction to the appropriate system cursor.
SDL_Cursor* cursor_for_resize(AppState* state, ResizeDir d);

/// SDL hit-test callback: marks titlebar as draggable, borders as resizable.
SDL_HitTestResult SDLCALL hit_test_callback(SDL_Window* win, const SDL_Point* area, void* data);
