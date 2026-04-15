#include <iostream>
#include <iomanip>
#include <cstring>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <cstdint>

#include "debugger.h"
#include "../common/color.h"

#include <capstone/capstone.h>

using namespace std;

// Init constructor class
MiniDebugger::MiniDebugger(pid_t pid)
    : m_pid(pid),
      m_bp_addr(0),
      m_bp_backup(0), m_bp_set(false)
{
}

// Function for viewing machine code
void MiniDebugger::read_memory(uint64_t addr, uint8_t *buf, size_t len)
{
    for (size_t i = 0; i < len; i += sizeof(long))
    {
        // Get value at memory location
        long word = ptrace(PTRACE_PEEKTEXT, m_pid, addr + i, nullptr);

        // If remaining less than 8 bytes, just read remaining
        size_t to_copy = min(sizeof(long), len - i);

        // Move read ones over
        memcpy(buf + i, &word, to_copy);
    }
}

uint64_t MiniDebugger::get_rip()
{
    struct user_regs_struct regs;

    // Get RIP
    ptrace(PTRACE_GETREGS, m_pid, nullptr, &regs);
    return regs.rip;
}

void MiniDebugger::set_breakpoint(uint64_t addr)
{
    m_bp_addr = addr;

    // Backup machine code at breakpoint
    m_bp_backup = ptrace(PTRACE_PEEKTEXT, m_pid, addr, nullptr);

    // Keep front, change last byte to 0, then to 0XCC
    long modified = (m_bp_backup & ~0xFF) | 0xcc;

    // Put back the modified to breakpoint memory
    ptrace(PTRACE_POKETEXT, m_pid, addr, modified);

    m_bp_set = true; // breakpoint set

    cout << Color::BOLD_LIME_GREEN << "[Debugger] Breakpoint set at 0x" << hex << addr << dec << Color::RESET << endl;
}

bool MiniDebugger::run_to_breakpoint()
{
    // Run until hit 0xcc
    ptrace(PTRACE_CONT, m_pid, nullptr, nullptr); //"CONT" run until breakpoint

    // Wait for hit 0xcc
    int status;
    waitpid(m_pid, &status, 0);

    if (WIFSIGNALED(status))
    {
        cout << "[Debugger] : Program exited with code " << WTERMSIG(status) << endl;
        return false;
    }

    // Write back the backup
    ptrace(PTRACE_POKETEXT, m_pid, m_bp_addr, m_bp_backup);

    // Get current RIP
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, m_pid, nullptr, &regs);

    // RIP pointer already jumped to next instruction, need to pull back and write back regs
    regs.rip = m_bp_addr;
    ptrace(PTRACE_SETREGS, m_pid, nullptr, &regs);

    cout << Color::BOLD_CORAL_RED << "[Debugger] Hit breakpoint at 0x" << hex << m_bp_addr << dec << Color::RESET << endl
         << endl;

    return true;
}

bool MiniDebugger::single_step()
{
    ptrace(PTRACE_SINGLESTEP, m_pid, nullptr, nullptr); // run one Assembly

    int status;
    waitpid(m_pid, &status, 0); // until it end

    if (WIFEXITED(status))
    {
        cout << "[Debugger] Program exited" << endl;
        return false;
    }

    return true;
}

void MiniDebugger::print_registers()
{
    struct user_regs_struct regs{};
    ptrace(PTRACE_GETREGS, m_pid, nullptr, &regs);

    static bool has_prev = false;
    static user_regs_struct prev_regs{};

    // Lambda expression to judge if register changed
    auto reg_color = [&](unsigned long long cur, unsigned long long prev, const char *highlight) -> const char *
    {
        if (!has_prev)
            return highlight;
        return (cur == prev) ? Color::BOLD_GREY : highlight;
    };

    cout << Color::BOLD_LAVENDER
         << "=== [ " << Color::YELLOW << "Registers" << Color::BOLD_LAVENDER
         << " ] ============================================================================"
         << Color::RESET << endl;

    cout << " RIP = " << reg_color(regs.rip, prev_regs.rip, Color::BOLD_YELLOW)
         << "0x" << hex << left << setw(12) << regs.rip << Color::RESET << "  ";
    cout << " RSP = " << reg_color(regs.rsp, prev_regs.rsp, Color::BOLD_CORAL_RED)
         << "0x" << left << setw(12) << regs.rsp << Color::RESET << "  ";

    cout << " RBP = " << reg_color(regs.rbp, prev_regs.rbp, Color::BOLD_CORAL_RED)
         << "0x" << left << setw(12) << regs.rbp << Color::RESET << "  ";
    cout << " RAX = " << reg_color(regs.rax, prev_regs.rax, Color::BOLD_ORANGE)
         << "0x" << left << setw(12) << regs.rax << Color::RESET << endl;

    cout << " RDI = " << reg_color(regs.rdi, prev_regs.rdi, Color::BOLD_DARK_BLUE)
         << "0x" << left << setw(12) << regs.rdi << Color::RESET << "  ";
    cout << " RSI = " << reg_color(regs.rsi, prev_regs.rsi, Color::BOLD_DARK_BLUE)
         << "0x" << left << setw(12) << regs.rsi << Color::RESET << "  ";

    cout << " RDX = " << reg_color(regs.rdx, prev_regs.rdx, Color::BOLD_DARK_BLUE)
         << "0x" << left << setw(12) << regs.rdx << Color::RESET << "  ";

    // Print ZF
    int zf = (regs.eflags >> 6) & 1ULL;
    cout << Color::BOLD_YELLOW << " ZF = " << zf << endl;

    cout << Color::RESET << dec << endl;

    // Remember to update prev_regs
    prev_regs = regs;
    has_prev = true;
}

void MiniDebugger::disassemble_at_rip(int count = 1)
{
    // RIP cannot be 0
    uint64_t rip = get_rip();
    if (rip == 0)
    {
        return;
    }

    uint8_t code[16 * count];

    // View machine code
    read_memory(rip, code, sizeof(code));

    // Init capstone and decide where to store result
    csh handle;
    cs_insn *insn;

    // Set capstone
    if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK)
    {
        cerr << "[Debugger] Capstone init failed" << endl;
        return;
    }

    // Start disassemble
    size_t n = cs_disasm(handle, code, sizeof(code), rip, count, &insn);

    if (n > 1)
    {
        cout << Color::YELLOW << "current line" << endl;
        cout << ">";
    }

    for (size_t i = 0; i < n; i++)
    {
        cout << Color::BOLD_DARK_BLUE << " 0x" << hex << insn[i].address << " : ";

        // Machine code
        cout << Color::BOLD_GREY;
        for (int j = 0; j < insn[i].size; j++)
        {
            cout << setw(2) << setfill('0') << (int)insn[i].bytes[j] << " "; // (int) change char to int
        }

        for (int j = insn[i].size; j < 8; j++)
        {
            cout << "   ";
        }

        // Output assembly language
        cout << Color::BOLD_LIGHT_RED << insn[i].mnemonic << " " << Color::BOLD_ORANGE << insn[i].op_str;

        cout << Color::RESET << dec << endl;
    }

    // Free memory (it is in heap, need manual free)
    cs_free(insn, n);
    cs_close(&handle);
}

void MiniDebugger::print_stack()
{
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, m_pid, nullptr, &regs);
    uint64_t rsp = regs.rsp;

    cout << Color::BOLD_LAVENDER << "=== [ " << Color::YELLOW << "Stack (top 10)" << Color::BOLD_LAVENDER
         << " ] ==================================================================================" << Color::RESET << endl;

    // Read 10 units of stack, each 8 bytes
    for (int i = 0; i < 10; i++)
    {
        uint64_t addr = rsp + i * 8;
        long val = ptrace(PTRACE_PEEKTEXT, m_pid, addr, nullptr);

        cout << " " << setfill(' ') << "[rsp + " << dec << right << setw(2) << i * 8 << "] "
             << Color::BOLD_DARK_BLUE << "0x" << setfill('0') << hex << setw(16) << addr << " : "
             << Color::BOLD_CORAL_RED << "0x" << setfill('0') << hex << setw(16) << val << Color::RESET << dec << setfill(' ');
        if ((i + 1) % 2 == 1)
        {
            cout << "  ";
        }
        else
        {
            cout << endl;
        }
    }
}

bool MiniDebugger::stepover(vector<Symbol> symbols, uint64_t base_address)
{
    // First get machine code
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, m_pid, nullptr, &regs);
    uint64_t rip = regs.rip;

    uint8_t buf[16];
    for (int i = 0; i < 16; i += 8)
    {
        long word = ptrace(PTRACE_PEEKDATA, m_pid, rip + i, nullptr);
        memcpy(buf + i, &word, 8);
    }

    // Prepare objdump
    csh handle;
    cs_open(CS_ARCH_X86, CS_MODE_64, &handle);

    cs_insn *insn;
    size_t count = cs_disasm(handle, buf, sizeof(buf), rip, 1, &insn);

    if (count == 0)
    {
        cs_close(&handle);
        return single_step();
    }

    // Check if call instruction
    bool isCall = (insn[0].id == X86_INS_CALL);

    // Place to return after call
    uint64_t nextaddr = rip + insn[0].size;
    uint64_t call_target = 0;
    if (buf[0] == 0xE8)
    {
        int32_t rel;
        memcpy(&rel, buf + 1, 4);
        call_target = rip + 5 + rel;
    }
    cs_free(insn, count);
    cs_close(&handle);

    if (isCall)
    {
        // First find from symbol, is there jump target function name, if in libc will not find
        for (auto &s : symbols)
        {
            if (s.offset + base_address == call_target)
            {
                cout << Color::BOLD_LIGHT_RED << "Call -> [ " << s.name << " ]" << endl;
                break;
            }
        }

        // If call, set breakpoint at end of call
        set_breakpoint(nextaddr);
        return run_to_breakpoint();
    }
    else
    {
        // If not, just step one
        return single_step();
    }
}