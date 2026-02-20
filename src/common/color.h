#pragma once

namespace Color{     //define colors
    const char* RESET = "\033[0m";  //default color
    const char* RED = "\033[91m";    //red
    const char* GREEN =  "\033[92m"; //green
    const char* YELLOW = "\033[93m"; //yellow
    const char* BLUE = "\033[94m";   //blue
    const char* MAGENTA = "\033[38;2;255;0;255m";  //magenta
    const char* CYAN = "\033[96m";  //cyan
    const char* WHITE = "\033[97m"; //white

    //bold text
    const char* BOLD_CORAL_RED = "\033[1;38;2;255;107;107m";
    const char* BOLD_LIME_GREEN = "\033[1;38;2;0;200;83m";
    const char* BOLD_ORANGE = "\033[1;38;2;255;167;38m";
    const char* BOLD_SKY_BLUE = "\033[1;38;2;100;181;246m";
    const char* BOLD_LAVENDER = "\033[1;38;2;186;104;200m";
    const char* BOLD_CYAN = "\033[1;38;2;100;181;246m";
    const char* BOLD_GREY = "\033[1;38;2;158;158;158m";
    const char* BOLD_WHITE = "\033[1;97m";
 }
