cmake_minimum_required(VERSION 3.16)
project(SDL_TradingPlatform CXX)

# =============================================================================
#  Build settings
# =============================================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Option: use FetchContent for dependencies (default ON)
option(DOWNLOAD_DEPENDENCIES "Download SDL3/nlohmann dependencies via FetchContent" ON)
# Option: build with Telegram notification support (default ON, requires libcurl)
option(ENABLE_TELEGRAM "Enable Telegram alert notifications via libcurl" ON)

# =============================================================================
#  Dependencies
# =============================================================================

if(DOWNLOAD_DEPENDENCIES)
    include(FetchContent)

    # ── SDL3 ──────────────────────────────────────────────────────────────────
    FetchContent_Declare(
        SDL3
        GIT_REPOSITORY "https://github.com/libsdl-org/SDL.git"
        GIT_TAG        "main"
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(SDL3)

    # ── SDL3_ttf ──────────────────────────────────────────────────────────────
    FetchContent_Declare(
        SDL3_ttf
        GIT_REPOSITORY "https://github.com/libsdl-org/SDL_ttf.git"
        GIT_TAG        "main"
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(SDL3_ttf)

    # ── SDL3_image ────────────────────────────────────────────────────────────
    FetchContent_Declare(
        SDL3_image
        GIT_REPOSITORY "https://github.com/libsdl-org/SDL_image.git"
        GIT_TAG        "main"
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(SDL3_image)

    # ── nlohmann/json ─────────────────────────────────────────────────────────
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

# ── libcurl (HTTP + Telegram) ──────────────────────────────────────────────────
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

    # ── Theme / UI ───────────────────────────────────────────────
    src/theme.cpp
    src/ui_utils.cpp

    # ── Networking ───────────────────────────────────────────────
    src/network.cpp

    # ── Persistence helpers ───────────────────────────────────────
    src/favorites.cpp

    # ── Chart module ─────────────────────────────────────────────
    src/chart.cpp

    # ── Market table (SPOT / FUTURES / FAV + search) ─────────────
    src/market_table.cpp

    # ── Order book module ─────────────────────────────────────────
    src/orderbook.cpp

    # ── Recent trades module ──────────────────────────────────────
    src/trades.cpp

    # ── Alerts engine + UI ────────────────────────────────────────
    src/alerts.cpp

    # ── Right panel (OB / Trades / Alerts tab switcher) ──────────
    src/right_panel.cpp
)

add_executable(sdl-trading ${TRADING_SOURCES})

# =============================================================================
#  Include directories
# =============================================================================

target_include_directories(sdl-trading PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# =============================================================================
#  Link libraries
# =============================================================================

target_link_libraries(sdl-trading PRIVATE
    SDL3::SDL3
    SDL3_ttf::SDL3_ttf
    SDL3_image::SDL3_image
    nlohmann_json::nlohmann_json
    CURL::libcurl
)

# Link math library on Unix
if(UNIX)
    target_link_libraries(sdl-trading PRIVATE m)
endif()

# =============================================================================
#  Compile definitions
# =============================================================================

target_compile_definitions(sdl-trading PRIVATE
    SDL_MAIN_USE_CALLBACKS
    $<$<BOOL:${ENABLE_TELEGRAM}>:ENABLE_TELEGRAM>
)

# =============================================================================
#  Compiler warnings
# =============================================================================

if(MSVC)
    target_compile_options(sdl-trading PRIVATE /W3)
else()
    target_compile_options(sdl-trading PRIVATE
        -Wall -Wextra -Wno-unused-parameter
    )
endif()

# =============================================================================
#  Platform-specific settings
# =============================================================================

# Windows: hide console window in release
set_property(TARGET sdl-trading PROPERTY WIN32_EXECUTABLE TRUE)

# Emscripten (WebAssembly)
if(EMSCRIPTEN)
    set_property(TARGET sdl-trading PROPERTY SUFFIX ".html")
    target_link_options(sdl-trading PRIVATE
        -sUSE_PTHREADS=1
        -sUSE_CURL=1
        -sPTHREAD_POOL_SIZE=8
    )
endif()

# =============================================================================
#  Build info printout
# =============================================================================

message(STATUS "=== SDL Trading Platform ===")
message(STATUS "  C++ Standard   : ${CMAKE_CXX_STANDARD}")
message(STATUS "  Download deps  : ${DOWNLOAD_DEPENDENCIES}")
message(STATUS "  Telegram alerts: ${ENABLE_TELEGRAM}")
message(STATUS "  Build type     : ${CMAKE_BUILD_TYPE}")
