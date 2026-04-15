#pragma once

namespace Color
{
    // Reset color
    constexpr const char *RESET = "\033[0m";

    // --- Standard series (thin) ---
    constexpr const char *RED = "\033[38;5;167m";     // Brick red (warm not dazzling)
    constexpr const char *GREEN = "\033[38;5;71m";    // Sage green (soft feel)
    constexpr const char *YELLOW = "\033[38;5;172m";  // Dark orange yellow (not dazzling)
    constexpr const char *BLUE = "\033[38;5;111m";    // Sky blue (fresh bright)
    constexpr const char *MAGENTA = "\033[38;5;170m"; // Soft pink purple (medium brightness)
    constexpr const char *CYAN = "\033[38;5;73m";     // Gray cyan (textured cool tone)
    constexpr const char *WHITE = "\033[38;5;250m";   // Light white gray (not dazzling white)

    // --- Bold series (bold) ---
    constexpr const char *BOLD_CORAL_RED = "\033[1;38;2;210;105;90m";   // Bold warm brick red (key tips)
    constexpr const char *BOLD_LIGHT_RED = "\033[1;38;5;197m";          // Bold bright red (warning/error)
    constexpr const char *BOLD_LIME_GREEN = "\033[1;38;2;120;180;100m"; // Bold olive green (success status)
    constexpr const char *BOLD_ORANGE = "\033[1;38;2;210;140;60m";      // Bold amber orange (note mark)
    constexpr const char *BOLD_DARK_BLUE = "\033[1;38;2;110;160;220m";  // Bold lake blue (slightly brighter version)
    constexpr const char *BOLD_LAVENDER = "\033[1;38;2;190;160;230m";   // Bold lavender purple (medium brightness)
    constexpr const char *BOLD_MAGENTA = "\033[1;38;2;180;100;160m";    // Bold berry pink (deep pink)
    constexpr const char *BOLD_YELLOW = "\033[1;38;5;178m";             // Bold gold yellow (slightly dimmed version)
    constexpr const char *BOLD_GREY = "\033[1;38;2;140;140;140m";       // Bold medium gray (auxiliary info)
    constexpr const char *BOLD_WHITE = "\033[1;38;2;220;220;220m";      // Bold soft white (emphasize text)
}