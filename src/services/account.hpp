#pragma once

#include "core/app_state.hpp"

// =============================================================================
//  Account — mock account simulation thread
// =============================================================================

/// Background thread that updates mock account balance/PNL for the titlebar.
void mock_account_thread(AppState* state);
