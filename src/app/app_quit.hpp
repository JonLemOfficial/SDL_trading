#pragma once

#include <SDL3/SDL.h>

// =============================================================================
//  App Quit — SDL_AppQuit callback (shutdown and cleanup)
// =============================================================================

/// Stops threads, saves persistence files, and releases all SDL resources.
void app_quit(void* appstate, SDL_AppResult result);
