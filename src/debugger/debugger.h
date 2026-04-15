#pragma once
#include <cstdint>
#include <sys/types.h>
#include <vector>
#include <sys/types.h>
#include "../symbols/symbols.h"

// Most need pid_t, so wrap in class
class MiniDebugger
{
public:
    MiniDebugger(pid_t pid);
    void set_breakpoint(uint64_t addr);                          // Set breakpoint
    bool run_to_breakpoint();                                    // Run until hit breakpoint
    bool single_step();                                          // Single step execution
    bool stepover(vector<Symbol> symbol, uint64_t base_address); // Smart single step that skips call
    void print_registers();                                      // Print registers
    void disassemble_at_rip(int count);                          // Disassemble current and future code
    void print_stack();                                          // Print stack

private:
    pid_t m_pid;
    uint64_t m_bp_addr;
    long m_bp_backup;
    bool m_bp_set;

    // Get value at target memory location
    void read_memory(uint64_t addr, uint8_t *buf, size_t len);

    // Get current RIP
    uint64_t get_rip();
};