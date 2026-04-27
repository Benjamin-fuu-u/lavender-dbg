# Lavender dbg
![Lavender Cover](https://hackmd.io/_uploads/HyjEgNmabe.png)

**Table of Contents**
* Introduction
* Quick Start
* How to Use
* Core Design and Code Analysis
* Actual Execution Demo
* Architecture Overview
* How You Can Expand
* Future Plans & Conclusion
  
# Introduction
**What is this**
Can high school students develop a debugger?
Lavender dbg is my answer. It is a basic debugger, running on Linux and implemented in C++, aiming to create a debugger that is easy to read and expand.

In addition, Lavender's main function is to help users analyze the logic behind the program, including maps output, ELF file parsing, and assembly output. For details, please refer to: **Core Features**

**GitHub Link:**https://github.com/Benjamin-fuu-u/lavender-dbg
**You can also see it on notion :** 

[https://www.notion.so/Lavender-dbg-34c01dbb4304802e9013d2ad249e60e2#34c01dbb430480c0a33dd079e71bdb6c](https://www.notion.so/Lavender-dbg-34c01dbb4304802e9013d2ad249e60e2?pvs=21)

Chinese (繁體中文) notion page

[https://www.notion.so/Lavender-dbg-34901dbb430480f1a237c7ad881f9cbe](https://www.notion.so/Lavender-dbg-34901dbb430480f1a237c7ad881f9cbe?pvs=21)

**This project was assisted by AI during production, and the author is not a native English speaker, so the writer ask AI to assist in translation.**

**Suitable for**

**Not suitable for**

- Beginners in system programming

- Users who need professional features

- Users who want to understand debugger principles

- Penetration testing or CTF competition environments
- People curious about underlying principles

**Highlights**

- Code is readable and easy to understand, clearly demonstrating the underlying principles of the debugger
- Clear architecture, easy to expand, can add or remove features according to your needs
- Lays the foundation for system programming

# Environment Requirements

**Environment Requirements**

- Operating System `Linux` (recommended `Linux MINT`)
- Compilation tools `g++(C++17)`, `gcc`
- Build tools `CMake 3.16+`
- External packages `capstone`, `objdump`

**File Setup**

```
lavender dbg/
├── CMakeLists.txt
└── src/
    ├── main.cpp
    ├── target.c
    └── ...
```

Please set up the files as shown above and install the following tools.

**Installation Commands**

```
sudo apt update
sudo apt install g++ gcc
sudo apt install cmake            #cmake
sudo apt install binutils         #objdump
sudo apt install libcapstone-dev  #capstone
```

Then create a `build` folder under lavender dbg

```
mkdir build
cd build
cmake ..  # First time must configure CMake, cmake is in the parent directory
make      #compile program
```

**In the future, if modifying CMake, enter `cmake`. If changing program content, enter `make`**

Run

```
./lavender ./target
```

# Core Features

Lavender dbg is a debugger used to help users understand the underlying principles behind programs, and can perform the following functions:
**1. Maps output**
At the beginning of the program, it prints the `maps` of the child process just started; you can also modify the settings to view during program runtime.
**2. Function address output**
The program automatically prints all internal, external, and system functions when the child process starts, but does not include dynamic link libraries like `libc.so`.
**3. CLI output; this program provides two functions: setting breakpoints and single-step executionBreakpoint setting**

**breakpoint** 
Can use hexadecimal or function names; after setting a breakpoint, it automatically goes to the breakpoint and displays `stack`, `register`, and the next five lines of assembly.

**Single-step execution**
Can customize the number of steps, and automatically skips when encountering `call`.

**You can refer to the actual execution demo**

**You can also write your own custom functions**

# Core Design and Code Analysis

**Starting Child Process**
The debugger and target program are two independent processes. Therefore, I use `fork()` to copy the child process, then use `execv()` to replace the child process with the target program, and finally let the child process call `PTRACE_TRACEME` to request the kernel to be traced by the parent.

```cpp
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
```

**Breakpoint Recovery Design**
When setting a breakpoint, I replace the first `byte` of the target address with `0xcc`, then use `PTRACE_CONT` to let the program run at full speed; when the CPU hits `0xcc`, it automatically pauses and triggers `SIGTRAP`.

**After the breakpoint is triggered, three things need to be done:**

1. Write back the backed-up machine code from when the breakpoint was set
2. Pull back the `RIP` (when the CPU executes to `0xcc`, `RIP` has already pointed to the next instruction)
3. Write back the current `registers`

```cpp
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
```

**Smart Stepover, Automatically Skip on Call**
In the stepover function design, it automatically skips encountered `call` functions and displays where to jump next. Therefore, I passed in the symbols storing each function address (obtained by `symbols.cpp`), then, due to ASLR randomization, the symbols store only offsets, and must add `base_address` to get the real address of the function.

**We need to do the following four things:**

1. Determine if the instruction is `call(X86_INS_CALL)`
2. If it is a call, set a breakpoint at the next instruction of this instruction (after jumping, `return` will come back here)
3. Check if there is a function name corresponding to the address in symbols **(if it is `libc` etc., it will show not found)**
4. Run to the breakpoint

```cpp
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
```

# Actual Execution Demo

**Maps Output**

![Maps Output](https://hackmd.io/_uploads/rk4BuP1pWg.png)

From the image above, you can see the maps, stack, vdso, code, and other sections at the beginning of program startup.

**Section Types**

| Item | Description |
| --- | --- |
| **code** | Mostly code |
| **stack** | Stack |
| **vdso** | Shared block, providing commonly used modules directly to the program |
| **heap** | Heap |
| **DATA** | Mostly storing data |
| **libc\.so** | Standard C library |

**Breakpoint Output**

![Breakpoint Main](https://hackmd.io/_uploads/rJTr_PJa-g.png)

This is the information displayed by Lavender when setting a breakpoint at main, including the next five lines of code (Current and next five instruction), registers, stack, and other information.

**Register Types**

| Register | Description | Register | Description |
| --- | --- | --- | --- |
| **RIP** | Current instruction position | **RDI** | Register 1 |
| **RSP** | Stack top | **RSI** | Register 2 |
| **RBP** | Stack bottom | **RDX** | Register 3 |
| **RAX** | Return value | **ZF** | Judgment flag |

**Single Step Output**

![Call Puts Print Hello](https://hackmd.io/_uploads/SkzLuDJTZg.png)

From the image above, you can see the machine code line by line; each line of machine code displays registers, with changes highlighted. If it is a call, it displays the called function name (but if it is libc etc., it will not find it, will not display function name).

**Also Supports Child Process Input**

![Lavender Input Name](https://hackmd.io/_uploads/SyJaHDzTWx.png)

This is the situation when the child process handled by Lavender has input; you can see inputting the name "lavender".

![Lavender Output Hello, Name](https://hackmd.io/_uploads/Hy_TSvf6We.png)

You can see outputting the name "lavender".

# Architecture Overview

**Project Architecture Diagram**

```
lavender-dbg/
├── CMakeLists.txt
└── src/
    ├── main.cpp             # Module integration and CLI
    ├── target.c             # Target program in C
    ├── common/
    │   └── color.h          # Output color definitions
    ├── debugger/            # Debugger module
    │   ├── debugger.cpp
    │   └── debugger.h
    ├── memory/              # /proc/maps reading
    │   ├── memory.cpp
    │   └── memory.h
    ├── process/             # Subprocess launching and control
    │   ├── process.cpp
    │   └── process.h
    └── symbols/             # ELF symbol parsing
        ├── symbols.cpp
        └── symbols.h
```

**Main Program Execution Flow**

**Program Startup**

1. User provides the debugged program name
2. `target.c` is compiled along with the build as the default debug target

**Child Process Creation**

```
Parent Process (lavender)
└─ fork()
     ├─ Child Process → execv (CMake automatically compiles target.c as the default debug target)
     │       + PTRACE_TRACEME  ← Requests the kernel to be traced by the parent
     └─ Parent Process → Waits for and controls the child process
```

**Reading Memory Mapping**

1. Read `/proc/[PID]/maps`
2. Obtain `maps` and `base_address`

**ELF File Parsing**

1. Use `objdump` to parse `ELF` file
2. Obtain function offsets

**CLI Interactive Output**

1. `breakpoint` After setting breakpoint, immediately executes to the breakpoint
2. Executes line by line; if encountering `call`, automatically queries the symbol table, looks up the jump function name, and sets breakpoint at the return location

**Program End**
After child process or user exits CLI, read the report left by child process, ensure to end child process, then end parent process

# How You Can Expand

**Current Limitations of This Project**

- Cannot directly modify the memory content of the target process
- Parent program currently shares the same terminal with child process; if using `step` to enter input function, may cause abnormality
- Uses parent program to start target child process; cannot `attach` to any process
- May have some functions missing or unstable

**How to Expand**

1. **Modify child process memory content**
Lavender currently can only read; can add `PTRACE_POKETEXT` to change memory
2. **Attach to any program**
Lavender currently uses fork(); you can change to `PTRACE_ATTACH`
3. **Custom CLI interface**
Commands in main.cpp are easy to replace and change
4. **Terminal separation**
Lavender currently has child and parent sharing the same terminal; you can use `dup2` to separate

# Future Plans & Conclusion

Lavender dbg is a debugger built in a simple and understandable way; piecing together the entire debugger by hand, the understanding of the underlying logic can be said to be very profound. In the future when using `dbg`, I think there will be more insights.

During the development process, I used AI to help query syntax, debug, and think about the overall program architecture. But I understand the meaning of every line of code, and believe that collaborating with AI will be the trend in the future.

I hope everyone can learn some underlying logic from this project, or become interested in underlying logic because of it, or even assemble a `dbg` by hand—of course, using AI to develop is also a good choice.

**GitHub Link:**https://github.com/Benjamin-fuu-u/lavender-dbg

If you like my project or my ideas, you can give me a star; that would be a great encouragement to me!
