#pragma once
#include <cstdint>
#include <sys/types.h>
#include <vector>
#include <sys/types.h>
#include "../symbols/symbols.h"

class MiniDebugger
{
public:
    MiniDebugger(pid_t pid); // get pid when the class start
    void set_breakpoint(uint64_t addr);
    bool run_to_breakpoint();
    bool single_step();
    bool stepover(vector<Symbol> symbol, uint64_t base_address);
    void print_registers();
    void disassemble_at_rip(int count);
    void print_stack();

private:
    pid_t m_pid;        // child program ID
    uint64_t m_bp_addr; // breakpoint address
    long m_bp_backup;   // breakpoint backup
    bool m_bp_set;      // set breakpoint [Y/N]

    void read_memory(uint64_t addr, uint8_t *buf, size_t len);
    // read where , where store the result(uint8_t is one byte) , read how many
    uint64_t get_rip(); // where the program(get RIP)
};