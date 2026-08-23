#pragma once

#include <SDL3/SDL.h>

// =============================================================================
//  App Init — SDL_AppInit callback (window, fonts, threads)
// =============================================================================

/// Initializes SDL, creates window/renderer, loads fonts and starts background threads.
SDL_AppResult app_init(void** appstate, int argc, char* argv[]);
