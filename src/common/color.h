#pragma once

namespace Color
{
    // 重設顏色
    constexpr const char *RESET = "\033[0m";

    // --- 標準系列 (細體) ---
    constexpr const char *RED = "\033[38;5;167m";     // 磚紅色 (溫潤不刺眼)
    constexpr const char *GREEN = "\033[38;5;71m";    // 鼠尾草綠 (柔和感)
    constexpr const char *YELLOW = "\033[38;5;214m";  // 亮橙黃 (高可讀性)
    constexpr const char *BLUE = "\033[38;5;111m";    // 天藍色 (清爽明亮)
    constexpr const char *MAGENTA = "\033[38;5;170m"; // 柔粉紫 (中等亮度)
    constexpr const char *CYAN = "\033[38;5;73m";     // 灰青色 (質感冷色調)
    constexpr const char *WHITE = "\033[38;5;250m";   // 淺白灰 (不刺眼的白)

    // --- Bold 系列 (加粗) ---
    constexpr const char *BOLD_CORAL_RED = "\033[1;38;2;210;105;90m";   // 粗體暖磚紅 (重點提示)
    constexpr const char *BOLD_LIGHT_RED = "\033[1;38;5;197m";          // 粗體亮紅 (警告/錯誤)
    constexpr const char *BOLD_LIME_GREEN = "\033[1;38;2;120;180;100m"; // 粗體橄欖綠 (成功狀態)
    constexpr const char *BOLD_ORANGE = "\033[1;38;2;210;140;60m";      // 粗體琥珀橘 (注意標記)
    constexpr const char *BOLD_DARK_BLUE = "\033[1;38;2;110;160;220m";  // 粗體湖水藍 (稍微調亮版)
    constexpr const char *BOLD_LAVENDER = "\033[1;38;2;190;160;230m";   // 粗體薰衣草紫 (中庸亮度)
    constexpr const char *BOLD_MAGENTA = "\033[1;38;2;180;100;160m";    // 粗體莓果粉 (深粉色)
    constexpr const char *BOLD_YELLOW = "\033[1;38;5;178m";             // 粗體金黃色 (微調降亮版)
    constexpr const char *BOLD_GREY = "\033[1;38;2;140;140;140m";       // 粗體中灰色 (輔助資訊)
    constexpr const char *BOLD_WHITE = "\033[1;38;2;220;220;220m";      // 粗體柔白色 (強調文字)
}