#pragma once

#include <SDL3/SDL.h>

// =============================================================================
//  App Render — SDL_AppIterate callback (frame draw loop)
// =============================================================================

/// Clears the frame, draws all UI layers, and presents to screen.
SDL_AppResult app_iterate(void* appstate);
