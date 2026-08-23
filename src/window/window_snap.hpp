#pragma once

#include "core/app_state.hpp"
#include "core/types.hpp"

#include <SDL3/SDL.h>

// =============================================================================
//  Window Snap — Aero Snap emulation for borderless windows
// =============================================================================

/// Returns usable bounds for the monitor containing the window.
SDL_Rect get_display_bounds(AppState* state);

/// Returns true when the window is in maximized mode (uses internal flag).
bool window_is_maximized(AppState* state);

/// Clamps and centers the window on first launch; sets initial restore rect.
void init_window_geometry(AppState* state);

/// Detects which snap zone the global mouse cursor is near (top/left/right).
SnapZone compute_snap_zone(AppState* state);

/// Returns the target screen-space rectangle for a snap zone.
SDL_Rect snap_target_rect(AppState* state, SnapZone zone);

/// Saves current window position/size into `win_restore_*` (never while maximized).
void save_window_restore_rect(AppState* state);

/// Expands the window to fill the usable display area.
void maximize_window_to_display(AppState* state);

/// Restores the window to the saved pre-maximize geometry.
void restore_window(AppState* state);

/// Toggles between maximized and restored using geometry + flag.
void toggle_window_maximize(AppState* state);

/// Begins tracking a titlebar drag for snap (restores if currently maximized).
void begin_snap_drag(AppState* state, float mx, float my);

/// Applies the given snap zone (resize/reposition window).
void apply_snap(AppState* state, SnapZone zone);

/// Polls mouse state during native titlebar drag; applies snap on release.
void update_snap_during_drag(AppState* state);

/// Draws a semi-transparent preview overlay for the active snap zone.
void draw_snap_overlay(AppState* state);
