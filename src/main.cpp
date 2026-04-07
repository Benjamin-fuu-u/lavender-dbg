#include <iostream>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <string>

#include "process/process.h"
#include "memory/memory.h"
#include "common/color.h"
#include "symbols/symbols.h"
#include "debugger/debugger.h"

using namespace std;

int main(int argc, char *argv[])
{
    using namespace std;
    if (argc != 2)
    {
        print_how_to_use(argv[0]); // argv[0] = program name
        exit(1);
    }
    const char *target_path = argv[1];
    pid_t child_pid = start_child_and_father_program(target_path);
    if (child_pid == -1)
    {
        cerr << "Error" << endl;
        exit(1);
    }

    vector<MemoryRegion> regions = read_memory_maps(child_pid);

    print_colored_maps(regions);

    uint64_t base = get_base_address(regions, target_path);
    vector<Symbol> symbols = read_symbols(target_path);
    cout << endl;
    print_symbol_address(symbols, base);

    MiniDebugger dbg(child_pid);

    string s;
    cout << Color::BOLD_LAVENDER
         << "=== [ " << Color::YELLOW << "USAGE" << Color::BOLD_LAVENDER
         << " ] ============================================================================================="
         << Color::RESET << endl;

    cout << " single (steps) or s (steps)                    : step through next (steps) instruction (it will jump call)" << endl;
    cout << " breakpoint (address or function name) or bp    : set breakpoint at function name or address (and run to the breakpoint)" << endl;
    cout << " quit or q                                      : quit the program" << endl;

    cout << endl
         << Color::BOLD_CORAL_RED << "Suggest set breakpoint at main " << Color::RESET << endl
         << endl;

    cout << Color::YELLOW << ">>" << Color::RESET;
    bool exited = false;
    while (getline(cin, s))
    {
        istringstream iss(s);
        string cmd;
        iss >> cmd;

        if (cmd == "quit" || cmd == "q")
        {
            break;
        }
        else if (cmd == "single" || cmd == "s")
        {
            string arg;
            if (!(iss >> arg))
            {
                if (!dbg.stepover())
                {
                    break;
                }
                dbg.disassemble_at_rip(1);
                dbg.print_registers();
                cout << Color::YELLOW << ">>" << Color::RESET;
                continue;
            }

            int times;
            try
            {
                times = stoi(arg);
            }
            catch(...)
            {
                cout<<"[Debugger] Error: invalied steps"<<endl;
                cout<<Color::YELLOW<<">>";
                continue;
            }
            
            for (int i = 0; i < times; i++)
            {
                if (!dbg.stepover())
                {
                    exited = true;
                    break;
                }
                dbg.disassemble_at_rip(1);
                dbg.print_registers();
            }
        }
        else if (cmd == "breakpoint" || cmd == "bp")
        {
            string arg;
            if (!(iss >> arg))
            {
                cout << "[Debugger] Error invaled function address or name" << endl;
                continue;
            }

            uint64_t bp_addr = 0;

            if (arg.size() > 2 && arg[0] == '0' && (arg[1] == 'x') || arg[1] == 'X') // if isʼs a function address itʼs start as 0x or 0X
            {
                try
                {
                    bp_addr = stoull(arg, nullptr, 16);
                } // try if it can change hex if not continue
                catch (...)
                {
                    cout << "[Debugger] Error invalied address : " << arg << endl;
                    continue;
                }
            }
            else
            {
                int64_t offset = find_symbol_offset(symbols, arg);
                if (offset < 0)
                {
                    cout << "[Debugger] Error: Symbol not found : " << arg << endl;
                    continue;
                }
                bp_addr = base + static_cast<uint64_t>(offset);
            }
            dbg.set_breakpoint(bp_addr);
            if (!dbg.run_to_breakpoint())
            {
                cout << "[Debugger] Program exited" << endl;
                break;
            }
            dbg.disassemble_at_rip(1);
            cout << Color::BOLD_LAVENDER
                 << "=== [ " << Color::YELLOW << "Current and next five instruction" << Color::BOLD_LAVENDER
                 << " ] ==============================================================="
                 << Color::RESET << endl;

            dbg.disassemble_at_rip(6);
            dbg.print_registers();
            dbg.print_stack();
        }
        else
        {
            cout << "[Error] : No such commmand" << endl;
        }
        cout << Color::YELLOW << ">>" << Color::RESET;

        if (exited)
        {
            break;
        }
    }

    clean_child_program(child_pid);
    return 0;
}
