#include "services/account.hpp"

#include <chrono>
#include <cstdlib>
#include <mutex>
#include <thread>

void mock_account_thread(AppState* state) {
  float mock_pnl = 15.50f;
  while (state->running) {
    {
      std::lock_guard<std::mutex> lock(state->account_mtx);
      state->account.balance = 12450.75f;
      state->account.open_trades = 2;
      state->account.pending_trades = 1;
      mock_pnl += ((rand() % 100) / 50.f) - 1.f;
      state->account.pnl = mock_pnl;
    }
    for (int i = 0; i < 10 && state->running; i++)
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}
