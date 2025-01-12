#pragma once

namespace skb {
void EnableProfileMode(int benchmark_index, int argument);
void DisableProfileMode();
bool IsProfileMode(int benchmark_index, int argumentZ);
}
