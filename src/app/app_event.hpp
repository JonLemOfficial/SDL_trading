#pragma once

#include <SDL3/SDL.h>

// =============================================================================
//  App Event — SDL_AppEvent callback (input routing)
// =============================================================================

/// Routes keyboard, mouse, and window events to the appropriate UI handlers.
SDL_AppResult app_event(void* appstate, SDL_Event* event);
