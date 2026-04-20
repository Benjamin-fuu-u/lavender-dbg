# Lavender dbg
![Lavender 封面](https://hackmd.io/_uploads/HyjEgNmabe.png)

**Table of Contents**

* Introduction
* Core Design and Code Analysis
* Actual Execution Demo
* Architecture Overview
* How You Can Expand It
* Quick Start
* Future Plans & Conclusion

# Introduction
**What is this **
Can high school students develop a debugger? Lavender dbg is my answer. It is a basic debugger that runs on Linux and is implemented in C++, aiming to create a debugger that is easy to read and expand.

**GitHub Link:**
https://github.com/Benjamin-fuu-u/lavender-dbg

**This project was assisted by AI during production, and the author is not a native English speaker, so please assist with AI translation**

**Suitable for:**

* Beginners in system programming
* Users who want to understand debugger principles
* People curious about underlying principles


**Not suitable for:**

* Users who need professional features
* Penetration testing or CTF competition environments


**Highlights**

* The code is easy to read and understand, clearly demonstrating the underlying principles of the debugger
* Clear architecture, easy to expand, functions can be added or removed according to personal needs
* Lays the foundation for system programming


**Core Functions:**

**Breakpoint Setting**
    
    Supports setting breakpoints via hexadecimal and function names
    After setting a breakpoint, it directly jumps to the breakpoint
    When a breakpoint is triggered, it automatically displays: the next 5 lines of disassembly, stack,and register information
    
**Single Step Execution**
    
    Supports custom step count
    Intelligently skips when encountering call
    
**Additionally, you can modify the code according to your needs to achieve custom functions**

# Code Highlights

**Launching Subprocess**
The debugger and the target program are two independent processes. Therefore, I use fork() to create a subprocess, then use execv() to replace the subprocess with the target program, and finally have the subprocess call PTRACE_TRACEME to request tracking from the parent process by the kernel.
~~~cpp=
 pid_t pid = fork();

// if pid = -1 , error occured
if (pid == -1)
{
    cerr << "Fork error" << strerror(errno) << endl;
    return -1;
}

// pid is 0 , this is child program
else if (pid == 0)
{
    // ask system to be traced by parents
    ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);
    char *args[] = {const_cast<char *>(target_path), nullptr};

    // fork into target_path state , if success , code below will not run
    // target path is for kernel , args for child program
    execv(target_path, args);

    cerr << "Errno : " << strerror(errno) << endl;
    exit(1);
}
~~~

**Breakpoint Recovery** 
Design When setting a breakpoint, I replace the first byte of the target address with 0xcc, then use PTRACE_CONT to let the program run at full speed. When the CPU hits 0xcc, it automatically pauses and triggers SIGTRAP.

After the breakpoint is triggered, three things need to be done:

1. Write back the backed-up machine code from when the breakpoint was set
2. Pull back the RIP (when the CPU executes to 0xcc, the RIP has already pointed to the next instruction)
3. Write back the current registers

~~~cpp=
// Run until hit 0xcc
ptrace(PTRACE_CONT, m_pid, nullptr, nullptr); 

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
~~~

**Smart Stepover:** 
Automatically Skip on Call In the stepover function design, it automatically skips encountered call functions and displays where it jumps next. Therefore, I passed in the symbols storing each function address (obtained from symbols.cpp), and due to ASLR randomization, the symbols stored are only offsets, and the base_address must be added to get the real function address.

**We need to do the following four things:**

1. Determine if the instruction is call (X86_INS_CALL)
2. If it is call, set a breakpoint at the next instruction after this one (after the jump, return will come back here)
3. Check in symbols if there is a function name corresponding to that address (if in libc, it will show not found)
4. Run to the breakpoint

~~~cpp=
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
~~~

# Actual Execution Demo

**Maps Output**
![maps輸出](https://hackmd.io/_uploads/rk4BuP1pWg.png)

**Breakpoint Output**
![breakpoint main](https://hackmd.io/_uploads/rJTr_PJa-g.png)

**Single Step Output** 
![call puts print hello](https://hackmd.io/_uploads/SkzLuDJTZg.png)

**Also Supports Subprocess Input**
![lavender 輸入名字](https://hackmd.io/_uploads/SyJaHDzTWx.png)

![lavender 輸出 hello ,名字](https://hackmd.io/_uploads/Hy_TSvf6We.png)

# Architecture Overview
**Project Architecture Diagram**
    
    lavender-dbg/
        ├── CMakeLists.txt
        └── src/
        ├── main.cpp             # Module integration and CLI
        ├── target.c             # C language target program
        ├── common/
        │   └── color.h          # Output color definitions
        ├── debugger/            # Debugger module
        │   ├── debugger.cpp
        │   └── debugger.h
        ├── memory/              # /proc/maps reading
        │   ├── memory.cpp
        │   └── memory.h
        ├── process/             # Subprocess startup and control
        │   ├── process.cpp
        │   └── process.h
        └── symbols/             # ELF symbol parsing
            ├── symbols.cpp
            └── symbols.h
    
**Program Startup**

1. User provides the name of the program to be debugged
2. target.c will be compiled along with the build as the default debugging target

**Subprocess Creation**
    
    Parent Process (lavender)
    └─ fork()
         ├─ Child Process → execv (CMake will automatically compile target.c as the default debugging target)
         │       + PTRACE_TRACEME  ← Request tracking from the kernel
         └─ Parent Process → Wait and control the child process
         
**Reading Memory Mapping**

1. Read /proc/[PID]/maps
2. Obtain maps and base_address


**ELF File Parsing**

1. Parse ELF file with objdump
2. Obtain function offsets


**CLI Interaction Output**

1. After setting a breakpoint, it immediately runs to the breakpoint
2. Execute line by line, if encountering call, automatically query the symbol table, look up the jump function name, and set a breakpoint at the return location


**Program End** 
After the subprocess or user exits the CLI, read the report left by the subprocess, ensure the subprocess ends, then end the parent process

# How You Can Expand It
**Current Limitations of This Project**

1. Cannot directly modify the memory content of the target process
2. The parent process currently shares the same terminal with the child process; if using step to enter input functions, it may cause abnormalities
3. Uses parent process to start target subprocess, cannot attach to arbitrary processes
4. Some functions may be missing or unstable


**How to Expand**

**Modify Child Process Memory Content** 
Lavender currently can only read; you can add PTRACE_POKETEXT to modify memory
**Attach to Arbitrary Programs**
Lavender currently uses fork(); you can change to PTRACE_ATTACH
**Custom CLI Interface**
Commands in main.cpp are easy to replace and modify
**Terminal Separation**
Lavender currently has child and parent processes sharing the same terminal; you can use dup2 to separate them

# Quick Start
**Environment Requirements**

* Operating System Linux (Recommended Linux MINT)
* Compilation Tools g++ (C++17), gcc
* Build Tools CMake 3.16+
* External Packages capstone, objdump

**File Setup**
    
    lavender dbg/
    ├── CMakeLists.txt                 
    └── src/
        ├── main.cpp
        ├── target.c 
        └── ...

Please set up the files as shown above, and install the following tools
    
**Installation Commands**
    
    sudo apt update
    sudo apt install g++ gcc          
    sudo apt install cmake            #cmake
    sudo apt install binutils         #objdump
    sudo apt install libcapstone-dev  #capstone

Then create a build folder under lavender dbg

    mkdir build
    cd build
    cmake .. # First time must set up CMake, cmake is in the parent directory
    make     #Compile the program

In the future, if modifying CMake, enter cmake. If changing program content, enter make

    ./lavender ./target  
    
# Conclusion
Lavender dbg is a debugger built in a simple and understandable way, piecing together the entire debugger by hand, which can be said to provide a very profound understanding of the underlying logic. In the future, when using dbg, I believe there will be more insights.

During the development process, I used AI to help me query syntax, debug, and think about the overall program architecture. But I understand the meaning of every line of code, and I believe that collaborating with AI will be a future trend.

I hope everyone can learn some underlying logic from this project, or become interested in underlying logic because of it, or even assemble a dbg by hand. Of course, using AI for development is also a good choice.

**GitHub Link:** 
https://github.com/Benjamin-fuu-u/lavender-dbg

If you like my project or my ideas, you can give me a star, that would be a great encouragement for me!
