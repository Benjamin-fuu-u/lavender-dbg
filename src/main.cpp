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

int main(int argc, char *argv[])
{
    using namespace std;

    // check arguments
    if (argc != 2)
    {
        print_how_to_use(argv[0]);
        exit(1);
    }

    // first argument : target path
    const char *target_path = argv[1];

    cout << Color::BOLD_LAVENDER << R"(
  _                         _             _____  ____   _____ 
 | |                       | |           |  __ \|  _ \ / ____|
 | |     __ ___   _____  __| | ___ _ __  | |  | | |_) | |  __ 
 | |    / _` \ \ / / _ \/ _` |/ _ \ '__| | |  | |  _ <| | |_ |
 | |___| (_| |\ V /  __/ (_| |  __/ |    | |__| | |_) | |__| |
 |______\__,_| \_/ \___|\__,_|\___|_|    |_____/|____/ \_____|
                                                              
                                                              
             [ mini debugger ]                                 
)" << Color::RESET
         << endl;

    // start child program and return child'spid
    pid_t child_pid = start_child_and_father_program(target_path);

    // if returns -1 , start failed
    if (child_pid == -1)
    {
        cerr << "child program start error" << endl;
        exit(1);
    }

    vector<MemoryRegion> regions = read_memory_maps(child_pid);

    // print maps in color
    print_colored_maps(regions);

    // ASLR randomizes address , each load is at a different position , get base address first
    uint64_t base = get_base_address(regions, target_path);

    // get function offsets
    vector<Symbol> symbols = read_symbols(target_path);

    cout << endl;

    // print function addresses (base + offset)
    print_symbol_address(symbols, base);

    // pass child_pid to debugger
    MiniDebugger dbg(child_pid);

    cout << Color::BOLD_LAVENDER
         << "=== [ " << Color::YELLOW << "USAGE" << Color::BOLD_LAVENDER
         << " ] ============================================================================================="
         << Color::RESET << endl;

    cout << " single (steps) or s (steps)                    : step through next (steps) instruction (it will jump call)" << endl;
    cout << " breakpoint (address or function name) or bp    : set breakpoint at function name or address (and run to the breakpoint)" << endl;
    cout << " quit or q                                      : quit the program" << endl;

    // if set breakpoint at main , can skip less important system functions , remind user here
    cout << endl
         << Color::BOLD_CORAL_RED << "Suggest set breakpoint at main " << Color::RESET << endl
         << endl;

    cout << Color::YELLOW << ">>" << Color::RESET;

    string s;
    bool exited = false;
    while (getline(cin, s))
    {
        // get first word of user input string
        istringstream iss(s);
        string cmd;
        iss >> cmd;

        // q to quit break out of user input loop
        if (cmd == "quit" || cmd == "q")
        {
            break;
        }

        // single step execution
        else if (cmd == "single" || cmd == "s")
        {
            string arg;

            // if user only inputs single or s
            if (!(iss >> arg))
            {
                if (!dbg.stepover(symbols, base))
                {
                    break;
                }

                // disassemble current line and print registers
                dbg.disassemble_at_rip(1);
                dbg.print_registers();
                cout << Color::YELLOW << ">>" << Color::RESET;
                continue;
            }

            // user inputs times , use try-catch to avoid program crash , fail goes to catch
            int times;
            try
            {
                times = stoi(arg);
            }
            catch (...)
            {
                cout << "[Debugger] Error: invalid steps" << endl;
                cout << Color::YELLOW << ">>" << Color::RESET;
                continue;
            }

            // step times , loop for times
            for (int i = 0; i < times; i++)
            {
                if (!dbg.stepover(symbols, base))
                {
                    // if stepover fails,return false
                    // use exited flag break out of whole CLI loop
                    exited = true;
                    break;
                }
                dbg.disassemble_at_rip(1);
                dbg.print_registers();
            }
        }

        // set breakpoint
        else if (cmd == "breakpoint" || cmd == "bp")
        {
            // must inpit function anme or address
            string arg;
            if (!(iss >> arg))
            {
                cout << "[Debugger] Error invaled function address or name" << endl;
                cout << Color::YELLOW << ">>" << Color::RESET;
                continue;
            }

            uint64_t bp_addr = 0;

            // if input string length > 2 and starts with 0x , judge as memory address
            if (arg.size() > 2 && arg[0] == '0' && (arg[1] == 'x') || arg[1] == 'X')
            {
                // convert to unsigned hex
                try
                {
                    bp_addr = stoull(arg, nullptr, 16);
                }
                catch (...)
                {
                    cout << "[Debugger] Error invalied address : " << arg << endl;
                    cout << Color::YELLOW << ">>" << Color::RESET;
                    continue;
                }
            }

            // is function name find function memory postion
            else
            {
                // for finding function memory offset(ALSR)
                int64_t offset = find_symbol_offset(symbols, arg);

                // memory address cannot be < 0
                if (offset < 0)
                {
                    cout << "[Debugger] Error: Symbol not found : " << arg << endl;
                    cout << Color::YELLOW << ">>" << Color::RESET;
                    continue;
                }

                // add base address to get real address
                bp_addr = base + static_cast<uint64_t>(offset);
            }

            // set breakpoint
            dbg.set_breakpoint(bp_addr);

            // run to breakpoint , if child program ends , return false  , break
            if (!dbg.run_to_breakpoint())
            {
                cout << "[Debugger] Program exited" << endl;
                break;
            }
            dbg.disassemble_at_rip(1);

            // disassemble line output
            cout << Color::BOLD_LAVENDER
                 << "=== [ " << Color::YELLOW << "Current and next five instruction" << Color::BOLD_LAVENDER
                 << " ] ==============================================================="
                 << Color::RESET << endl;

            dbg.disassemble_at_rip(6);
            dbg.print_registers();
            dbg.print_stack();
        }

        // invalied command
        else
        {
            cout << "[Error] : No such command" << endl;
            cout << Color::YELLOW << ">>" << Color::RESET;
            continue;
        }

        // when stepover hits brealpoint , break here
        if (exited)
        {
            break;
        }
    }
    // close child program
    clean_child_program(child_pid);
    return 0;
}