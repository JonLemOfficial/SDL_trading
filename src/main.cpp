// =============================================================================
//  Trading Platform C++ — SDL3 Entry Point
//
//  Thin wrapper that forwards SDL3 main callbacks to the app/ modules.
//  See docs/ARCHITECTURE.md for the full module map.
// =============================================================================

#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>

#include "app/app_event.hpp"
#include "app/app_init.hpp"
#include "app/app_quit.hpp"
#include "app/app_render.hpp"

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
  return app_init(appstate, argc, argv);
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
  return app_event(appstate, event);
}

SDL_AppResult SDL_AppIterate(void* appstate) {
  return app_iterate(appstate);
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
  app_quit(appstate, result);
}
