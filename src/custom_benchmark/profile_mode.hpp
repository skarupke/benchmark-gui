#pragma once
#include <stdint.h>

namespace skb {
void EnableProfileMode(int benchmark_index, int64_t argument);
void DisableProfileMode();
bool IsProfileMode(int benchmark_index, int64_t argument);
}
