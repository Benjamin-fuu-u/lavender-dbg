#pragma once

namespace Color
{                                                           // define colors
    constexpr const char *RESET = "\033[0m";                // default color
    constexpr const char *RED = "\033[91m";                 // red
    constexpr const char *GREEN = "\033[92m";               // green
    constexpr const char *YELLOW = "\033[93m";              // yellow
    constexpr const char *BLUE = "\033[94m";                // blue
    constexpr const char *MAGENTA = "\033[38;2;255;0;255m"; // magenta
    constexpr const char *CYAN = "\033[96m";                // cyan
    constexpr const char *WHITE = "\033[97m";               // white

    // bold text
    constexpr const char *BOLD_CORAL_RED = "\033[1;38;2;255;107;107m";
    constexpr const char *BOLD_LIME_GREEN = "\033[1;38;2;0;200;83m";
    constexpr const char *BOLD_ORANGE = "\033[1;38;2;255;167;38m";
    constexpr const char *BOLD_SKY_BLUE = "\033[1;38;2;100;181;246m";
    constexpr const char *BOLD_LAVENDER = "\033[1;38;2;186;104;200m";
    constexpr const char *BOLD_CYAN = "\033[1;38;2;100;181;246m";
    constexpr const char *BOLD_GREY = "\033[1;38;2;158;158;158m";
    constexpr const char *BOLD_WHITE = "\033[1;97m";
}
