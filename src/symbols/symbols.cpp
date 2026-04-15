#include <iostream>
#include <iomanip>
#include <cstdio>
#include <cstring>
#include <algorithm>

#include "symbols.h"
#include "../common/color.h"

using namespace std;

// Specially read user-written functions
static void read_local_symbols(const string &path, vector<Symbol> &result)
{
    // Prepare command to open objdump, 2>/dev/null does not show error messages
    string cmd = "objdump -t -- " + path + " 2>/dev/null";

    // Open a pipe for data transfer to read
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe)
    {
        cerr << "[objdump]: error failed to run objdump" << endl;
        return;
    }

    char line[512];

    // fgets reads until EOF or newline, but it can only store result in char[]
    while (fgets(line, sizeof(line), pipe))
    {
        // Only function parts have "F"
        if (strstr(line, "F") == nullptr)
            continue;

        // Read starting memory position, sscanf is for reading string
        uint64_t addr = 0;
        if (sscanf(line, "%lx", &addr) != 1 || addr == 0)
            continue;

        // Convert to C++ string
        string sline(line);
        while (!sline.empty() && sline.back() == '\n')
            sline.pop_back();

        // rfind = reverse find, search space from the end
        size_t pos = sline.rfind(' ');
        if (pos == string::npos)
            continue;

        // Function name is after the last space
        string name = sline.substr(pos + 1);
        if (!name.empty())
            result.push_back({name, addr, false});
    }

    // Close the data transfer pipe just created
    pclose(pipe);
}

// Specially read external functions, like printf
static void read_plt_symbols(const string &path, vector<Symbol> &result)
{
    string cmd = "objdump -d -- " + path + " 2>/dev/null";
    FILE *pipe = popen(cmd.c_str(), "r");

    if (!pipe)
    {
        cerr << "[objdump] : error" << endl;
        return;
    }

    char line[512];

    while (fgets(line, sizeof(line), pipe))
    {
        // External functions have @plt
        if (strstr(line, "@plt>:") == nullptr)
            continue;

        uint64_t addr = 0;

        if (sscanf(line, "%lx", &addr) != 1 || addr == 0)
            continue;

        // For external functions, function name is between <>
        char *start = strchr(line, '<');
        char *end = strchr(line, '>');

        if (!start || !end || end <= start)
            continue;

        string name(start + 1, end - start - 1); // name = srart + 1 ~end - start - 1
        result.push_back({name, addr, true});
    }
    pclose(pipe);
}

// View symbol table, includes finding external and internal functions
vector<Symbol> read_symbols(const string &target_path)
{
    vector<Symbol> result;
    read_local_symbols(target_path, result);
    read_plt_symbols(target_path, result);

    if (result.empty())
        cerr << "[Symbols] : no symbol found" << endl;

    return result;
}

// Print the symbol table
void print_symbol_address(const vector<Symbol> &symbols, uint64_t base_address)
{
    cout << Color::BOLD_LAVENDER
         << "=== [ " << Color::YELLOW << "Function Address" << Color::BOLD_LAVENDER
         << " ] ==================================================================================="
         << Color::RESET << endl;

    cout << "function address         function name" << endl;

    // Sort by offset
    vector<Symbol> sorted = symbols;
    sort(sorted.begin(), sorted.end(),
         [](const Symbol &a, const Symbol &b)
         { return a.offset < b.offset; });

    for (const auto &s : sorted)
    {
        uint64_t actual = base_address + s.offset;

        cout << Color::BOLD_DARK_BLUE << " 0x" << hex << left << setw(16) << actual;
        cout << Color::RESET << " : ";

        // is_plt in struct Symbol, external function
        if (s.is_plt)
            cout << Color::BOLD_LIME_GREEN;

        // Starts with _, internal function
        else if (s.name[0] == '_')
            cout << Color::BOLD_GREY;
        // This is the function you wrote
        else
            cout << Color::BOLD_CORAL_RED;

        cout << s.name << Color::RESET << endl;
    }
    cout << dec << Color::RESET << endl;
}

int64_t find_symbol_offset(const vector<Symbol> &symbols, string &name) // find current function address
{
    // Find the function name and return address
    for (const auto &s : symbols)
    {
        if (s.name == name)
            return s.offset;
    }
    return -1;
}