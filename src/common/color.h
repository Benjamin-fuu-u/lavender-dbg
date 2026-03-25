#pragma once

namespace Color
{
    constexpr const char *RESET = "\033[0m";
    // 降低亮度，選擇更深一點的色號
    constexpr const char *RED = "\033[38;5;124m";    // 深紅
    constexpr const char *GREEN = "\033[38;5;22m";   // 森林綠
    constexpr const char *YELLOW = "\033[38;5;136m"; // 暗金/土黃
    constexpr const char *BLUE = "\033[38;5;26m";    // 鋼鐵藍
    constexpr const char *MAGENTA = "\033[38;5;90m"; // 深紫
    constexpr const char *CYAN = "\033[38;5;30m";    // 暗青色
    constexpr const char *WHITE = "\033[38;5;244m";  // 中灰色

    // Bold 系列：將 RGB 數值減半左右，保留色調但降低亮度
    constexpr const char *BOLD_CORAL_RED = "\033[1;38;2;180;70;70m"; // 磚紅
    constexpr const char *BOLD_LIME_GREEN = "\033[1;38;2;0;130;50m"; // 暗綠
    constexpr const char *BOLD_ORANGE = "\033[1;38;2;180;100;20m";   // 赭石
    constexpr const char *BOLD_DARK_BLUE = "\033[1;38;5;25m";        // 深海藍
    constexpr const char *BOLD_LAVENDER = "\033[1;38;2;120;70;130m"; // 灰紫
    constexpr const char *BOLD_MAGENTA = "\033[1;38;2;150;0;150m";   // 莓果紫
    constexpr const char *BOLD_YELLOW = "\033[1;38;5;100m";          // 橄欖黃
    constexpr const char *BOLD_GREY = "\033[1;38;2;100;100;100m";    // 深灰
    constexpr const char *BOLD_WHITE = "\033[1;38;5;248m";           // 淺灰（取代純白）
}