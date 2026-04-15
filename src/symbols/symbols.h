#pragma once
#include <string>
#include <vector>
#include <cstdint>

using namespace std;

// Data structure to store symbol table, has name and address
struct Symbol
{
    string name;
    uint64_t offset;
    bool is_plt;
};

// Open objdump, get ELF file, internal private functions division
vector<Symbol> read_symbols(const string &target_path);

// Print function name and position
void print_symbol_address(const vector<Symbol> &symbols, uint64_t base_address);

// Given function name, find address
int64_t find_symbol_offset(const vector<Symbol> &symbols, string &name);