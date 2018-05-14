#pragma once

#include <gtest/gtest.h>

#ifdef RUN_SLOW_TESTS
#define SLOW_TEST(x) x
#else
#define SLOW_TEST(x) DISABLED_ ## x
#endif

#define CURRENTLY_NOT_WORKING(x) DISABLED_ ## x
