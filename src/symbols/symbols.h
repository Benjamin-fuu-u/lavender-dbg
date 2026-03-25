#pragma
#include <string>
#include <vector>
#include <cstdint>

using namespace std;

struct Symbol
{
    string name;
    uint64_t offset;
    bool is_plt;
};

vector<Symbol> read_symbols(const string &target_path);

void print_symbol_address(const vector<Symbol> &symbols, uint64_t base_address);

int64_t find_symbol_offset(const vector<Symbol> &symbols, string &name);
