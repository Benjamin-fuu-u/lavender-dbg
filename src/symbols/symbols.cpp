#include <iostream>
#include <iomanip>
#include <cstdio>
#include <cstring>
#include <algorithm>

#include "symbols.h"
#include "../common/color.h"

using namespace std;

static void read_local_symbols(const string &path, vector<Symbol> &result)
{
    string cmd = "objdump -t -- " + path + " 2>/dev/null"; //" 2>/dev/null" donʼt show the error message , 2 is the error message , 1 is the normal output
    FILE *pipe = popen(cmd.c_str(), "r");                  // build a pipe to read the result
    if (!pipe)
    {
        cerr << "[objump]: error" << endl;
        return;
    }

    char line[512];

    while (fgets(line, sizeof(line), pipe))
    { // keep read until EOF or \n or 512 char
        if (strstr(line, "F") == nullptr)
            continue; // not function

        uint64_t addr = 0;
        if (sscanf(line, "%lx", &addr) != 1 || addr == 0)
            continue; // sscanf read the file store the result to &addr , != 1 canʼt read ,  addr == 0 (not use value)

        string sline(line); // change to C++
        while (!sline.empty() && sline.back() == '\n')
            sline.pop_back();

        size_t pos = sline.rfind(' ');
        if (pos == string::npos)
            continue; // canʼt find space

        string name = sline.substr(pos + 1); // cut the string
        if (!name.empty())
            result.push_back({name, addr, false});
    }

    pclose(pipe);
}

static void read_plt_symbols(const string &path, vector<Symbol> &result)
{
    string cmd = "objdump -d -- " + path + " 2>/dev/null";
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe)
    {
        cerr << "[objump] : error" << endl;
        return;
    }
    char line[512];
    while (fgets(line, sizeof(line), pipe))
    {
        if (strstr(line, "@plt>:") == nullptr)
            continue;

        uint64_t addr = 0;

        if (sscanf(line, "%lx", &addr) != 1 || addr == 0)
            continue;

        char *start = strchr(line, '<'); // function name between "< >"
        char *end = strchr(line, '>');

        if (!start || !end || end <= start)
            continue;

        string name(start + 1, end - start - 1); // name = srart + 1 ~end - start - 1
        result.push_back({name, addr, true});
    }
    pclose(pipe);
}

vector<Symbol> read_symbols(const string &target_path)
{
    vector<Symbol> result;
    read_local_symbols(target_path, result);
    read_plt_symbols(target_path, result);

    if (result.empty())
        cerr << "[Symbols] : no symbol found" << endl;

    return result;
}

void print_symbol_address(const vector<Symbol> &symbols, uint64_t base_address)
{
    cout << Color::BOLD_LAVENDER
         << "=== [ " << Color::YELLOW << "Function Address" << Color::BOLD_LAVENDER
         << " ] ==================================================================================="
         << Color::RESET << endl;

    cout << "function address         function name" << endl;

    vector<Symbol> sorted = symbols;
    sort(sorted.begin(), sorted.end(),
         [](const Symbol &a, const Symbol &b)
         { return a.offset < b.offset; });

    for (const auto &s : sorted)
    {
        uint64_t actual = base_address + s.offset;

        cout << Color::BOLD_DARK_BLUE << " 0x" << hex << left << setw(16) << actual;
        cout << Color::RESET << " : ";
        if (s.is_plt)
            cout << Color::BOLD_LIME_GREEN;

        else if (s.name[0] == '_')
            cout << Color::BOLD_GREY;
        else
            cout << Color::BOLD_CORAL_RED;

        cout << s.name << Color::RESET << endl;
    }
    cout << dec << Color::RESET << endl;
}

int64_t find_symbol_offset(const vector<Symbol> &symbols, string &name) // find current function affresss
{
    for (const auto &s : symbols)
    {
        if (s.name == name)
            return s.offset;
    }
    return -1;
}