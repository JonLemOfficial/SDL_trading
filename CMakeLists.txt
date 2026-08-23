cmake_minimum_required(VERSION 3.16)
project(SDL_TradingPlatform CXX)

# =============================================================================
#  Build settings
# =============================================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(DOWNLOAD_DEPENDENCIES "Download SDL3/nlohmann dependencies via FetchContent" ON)
option(ENABLE_TELEGRAM "Enable Telegram alert notifications via libcurl" ON)

# =============================================================================
#  Dependencies
# =============================================================================

if(DOWNLOAD_DEPENDENCIES)
    include(FetchContent)

    FetchContent_Declare(
        SDL3
        GIT_REPOSITORY "https://github.com/libsdl-org/SDL.git"
        GIT_TAG        "main"
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(SDL3)

    FetchContent_Declare(
        SDL3_ttf
        GIT_REPOSITORY "https://github.com/libsdl-org/SDL_ttf.git"
        GIT_TAG        "main"
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(SDL3_ttf)

    FetchContent_Declare(
        SDL3_image
        GIT_REPOSITORY "https://github.com/libsdl-org/SDL_image.git"
        GIT_TAG        "main"
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(SDL3_image)

    FetchContent_Declare(
        json
        URL          "https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz"
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(json)

else()
    find_package(SDL3       CONFIG REQUIRED)
    find_package(SDL3_ttf   CONFIG REQUIRED)
    find_package(SDL3_image CONFIG REQUIRED)
    find_package(nlohmann_json 3.12.0 REQUIRED)
endif()

find_package(CURL REQUIRED)

# =============================================================================
#  Platform: Android / iOS → shared library instead of executable
# =============================================================================

if(ANDROID OR IOS)
    function(add_executable TARGET)
        add_library(${TARGET} SHARED ${ARGN})
    endfunction()
endif()

# =============================================================================
#  Source files
# =============================================================================

set(TRADING_SOURCES
    # ── Entry point ──────────────────────────────────────────────
    src/main.cpp

    # ── App lifecycle (SDL3 callbacks) ───────────────────────────
    src/app/app_init.cpp
    src/app/app_event.cpp
    src/app/app_render.cpp
    src/app/app_quit.cpp

    # ── Core ─────────────────────────────────────────────────────
    src/core/layout.cpp

    # ── Window management ────────────────────────────────────────
    src/window/titlebar.cpp
    src/window/window_snap.cpp
    src/window/window_hit_test.cpp

    # ── UI ───────────────────────────────────────────────────────
    src/ui/theme.cpp
    src/ui/ui_utils.cpp
    src/ui/app_tabs.cpp
    src/ui/toolbox.cpp
    src/ui/chrome.cpp

    # ── Services ─────────────────────────────────────────────────
    src/services/network.cpp
    src/services/favorites.cpp
    src/services/account.cpp

    # ── Feature modules ──────────────────────────────────────────
    src/modules/chart.cpp
    src/modules/market_table.cpp
    src/modules/orderbook.cpp
    src/modules/trades.cpp
    src/modules/alerts.cpp
    src/modules/right_panel.cpp
)

add_executable(sdl-trading ${TRADING_SOURCES})

target_include_directories(sdl-trading PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(sdl-trading PRIVATE
    SDL3::SDL3
    SDL3_ttf::SDL3_ttf
    SDL3_image::SDL3_image
    nlohmann_json::nlohmann_json
    CURL::libcurl
)

if(UNIX)
    target_link_libraries(sdl-trading PRIVATE m)
endif()

target_compile_definitions(sdl-trading PRIVATE
    SDL_MAIN_USE_CALLBACKS
    $<$<BOOL:${ENABLE_TELEGRAM}>:ENABLE_TELEGRAM>
)

if(MSVC)
    target_compile_options(sdl-trading PRIVATE /W3)
else()
    target_compile_options(sdl-trading PRIVATE
        -Wall -Wextra -Wno-unused-parameter
    )
endif()

set_property(TARGET sdl-trading PROPERTY WIN32_EXECUTABLE TRUE)

if(EMSCRIPTEN)
    set_property(TARGET sdl-trading PROPERTY SUFFIX ".html")
    target_link_options(sdl-trading PRIVATE
        -sUSE_PTHREADS=1
        -sUSE_CURL=1
        -sPTHREAD_POOL_SIZE=8
    )
endif()

message(STATUS "=== SDL Trading Platform ===")
message(STATUS "  C++ Standard   : ${CMAKE_CXX_STANDARD}")
message(STATUS "  Download deps  : ${DOWNLOAD_DEPENDENCIES}")
message(STATUS "  Telegram alerts: ${ENABLE_TELEGRAM}")
message(STATUS "  Build type     : ${CMAKE_BUILD_TYPE}")
