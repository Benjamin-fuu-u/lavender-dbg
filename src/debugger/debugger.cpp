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

MiniDebugger::MiniDebugger(pid_t pid)
    : m_pid(pid) // initialization m_pid to pid
      ,
      m_bp_addr(0) // define the variable
      ,
      m_bp_backup(0), m_bp_set(false)
{
}

void MiniDebugger::read_memory(uint64_t addr, uint8_t *buf, size_t len)
{
    for (size_t i = 0; i < len; i += sizeof(long))
    {
        long word = ptrace(PTRACE_PEEKTEXT, m_pid, addr + i, nullptr);
        // get child program address , long means 8 bytes
        size_t to_copy = min(sizeof(long), len - i);
        // choose how many data sshould move(the ptrace read 8 byte and the last space) , choose the small to move
        memcpy(buf + i, &word, to_copy); // move word to buf , move to_copy size
    }
}

uint64_t MiniDebugger::get_rip()
{
    struct user_regs_struct regs; // the struct defined by linux (all the rip)
    ptrace(PTRACE_GETREGS, m_pid, nullptr, &regs);

    return regs.rip;
}

void MiniDebugger::set_breakpoint(uint64_t addr)
{
    m_bp_addr = addr;

    m_bp_backup = ptrace(PTRACE_PEEKTEXT, m_pid, addr, nullptr);
    // back up origin machine code 8 byte like 0x1122334455667788
    // and the start is 88 second is 77

    long modified = (m_bp_backup & ~0xFF) | 0xcc;
    // change the last byte (0x1122334455667788) to 0x1122334455667700  and fill cc
    // 0x11223344556677cc

    ptrace(PTRACE_POKETEXT, m_pid, addr, modified); // write the chanf address back

    m_bp_set = true; // breakpoint set

    cout << Color::BOLD_LIME_GREEN << "[Debugger] Breakpoint set at 0x" << hex << addr << dec << Color::RESET << endl;
}

bool MiniDebugger::run_to_breakpoint()
{
    ptrace(PTRACE_CONT, m_pid, nullptr, nullptr); //"CONT" run until breakpoint

    int status;
    waitpid(m_pid, &status, 0); // wait when the child program send single (0xcc or error)

    if (WIFSIGNALED(status))
    {
        cout << "[Debugger] : Program exited with code " << WTERMSIG(status) << endl;
        return false;
    }

    //***  important
    ptrace(PTRACE_POKETEXT, m_pid, m_bp_addr, m_bp_backup); // write origin command back

    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, m_pid, nullptr, &regs); // get the breakpoint regs
    regs.rip = m_bp_addr;                          // the kernel has point the 0xcc next Assembly changed it to the breakpoint addrss
    ptrace(PTRACE_SETREGS, m_pid, nullptr, &regs); // write the regs back

    cout << Color::BOLD_CORAL_RED << "[Debugger] Hit breakpoint at ox" << hex << m_bp_addr << dec << Color::RESET << endl
         << endl;

    // ptrace(PTRACE_SYSCALL, m_pid, nullptr, nullptr);
    return true;
}

bool MiniDebugger::single_step()
{
    ptrace(PTRACE_SINGLESTEP, m_pid, nullptr, nullptr); // run one Assembly

    int status;
    waitpid(m_pid, &status, 0); // until it end

    if (WIFEXITED(status))
    {
        cout << "[Debugger] Program existed" << endl;
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

    auto reg_color = [&](unsigned long long cur, unsigned long long prev, const char *highlight) -> const char *
    {
        if (!has_prev)
            return highlight;
        return (cur == prev) ? Color::BOLD_GREY : highlight;
    };

    cout << Color::BOLD_LAVENDER
         << "=== [ " << Color::YELLOW << "Registers" << Color::BOLD_LAVENDER
         << " ] ======================================================================================= "
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

    int zf = (regs.eflags >> 6) & 1ULL;
    cout << Color::BOLD_YELLOW << " ZF = " << zf << endl;

    cout << Color::RESET << dec << endl;
    prev_regs = regs;
    has_prev = true;
}

void MiniDebugger::disassemble_at_rip(int count = 1)
{
    uint64_t rip = get_rip();
    if (rip == 0)
    {
        return;
    }

    uint8_t code[16 * count];
    read_memory(rip, code, sizeof(code)); // read the next assembly (16 bytes) to code

    csh handle;    // innitilize capstone
    cs_insn *insn; // store the disassembly result int insn

    if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK) // use ARCHX86 64 bit if error exit
    {
        cerr << "[Debugger] Capstone init failed" << endl;
        return;
    }

    size_t n = cs_disasm(handle, code, sizeof(code), rip, count, &insn);
    // handle capstone , code the disassemble code ,rip use whem jump or call , 1 one assemble , insn store the result

    if (n > 1)
    {
        cout << Color::YELLOW << "current line" << endl;
        cout << ">";
    }

    for (size_t i = 0; i < n; i++)
    {                                                                              // disassembly OK
        cout << Color::BOLD_DARK_BLUE << " 0x" << hex << insn[i].address << " : "; // now instruction address

        cout << Color::BOLD_GREY;
        for (int j = 0; j < insn[i].size; j++) // how many machine instruction
        {
            cout << setw(2) << setfill('0') << (int)insn[i].bytes[j] << " "; //(int) change char to int
        }

        for (int j = insn[i].size; j < 8; j++) // if itʼs not 8 byte fill it
        {
            cout << "   ";
        }

        cout << Color::BOLD_LIGHT_RED << insn[i].mnemonic << " " << Color::BOLD_ORANGE << insn[i].op_str; // insn.mnemonic  like mov call , insn.op_str is the operater object
        cout << Color::RESET << dec << endl;
    }
    cs_free(insn, n); // memory release
    cs_close(&handle);
}

void MiniDebugger::print_stack()
{
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, m_pid, nullptr, &regs);
    uint64_t rsp = regs.rsp;

    cout << Color::BOLD_LAVENDER << "=== [ " << Color::YELLOW << "Stack (top 10)" << Color::BOLD_LAVENDER
         << " ] ==================================================================================" << Color::RESET << endl;
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
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, m_pid, nullptr, &regs);
    uint64_t rip = regs.rip;

    uint8_t buf[16];
    for (int i = 0; i < 16; i += 8)
    {
        long word = ptrace(PTRACE_PEEKDATA, m_pid, rip + i, nullptr);
        memcpy(buf + i, &word, 8);
    }

    csh handle;
    cs_open(CS_ARCH_X86, CS_MODE_64, &handle);

    cs_insn *insn;
    size_t count = cs_disasm(handle, buf, sizeof(buf), rip, 1, &insn);

    if (count == 0)
    {
        cs_close(&handle);
        return single_step();
    }

    bool isCall = (insn[0].id == X86_INS_CALL);
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
        for (auto &s : symbols)
        {
            if (s.offset + base_address == call_target)
            {
                cout << Color::BOLD_LIGHT_RED << "Call -> [ " << s.name << " ]" << endl;
                break;
            }
        }
        set_breakpoint(nextaddr);
        return run_to_breakpoint();
    }
    else
    {
        return single_step();
    }
}