#pragma once

#include "../framework/test_framework.h"

#include <railguard/core/window.h>

TEST(WindowBasicInitialization) {
    // Create a window
    rg_window *window = rg_create_window((rg_extent_2d){800, 600}, "Test Window");
    ASSERT_NOT_NULL(window);

    // Test extent
    rg_extent_2d extent = rg_window_get_current_extent(window);
    EXPECT_TRUE(extent.width == 800);
    EXPECT_TRUE(extent.height == 600);

    // Destroy the window
    rg_destroy_window(&window);
    ASSERT_NULL(window);
}