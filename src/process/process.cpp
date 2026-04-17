#include <iostream>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

#include "process.h"
#include "../common/color.h"

using namespace std;

void print_how_to_use(const char *program_name)
{
    using namespace std;
    cerr << "Error " << program_name << " need the target path" << endl;
    cerr << "For example : " << program_name << " ./hello" << endl;
    return;
}

pid_t start_child_and_father_program(const char *target_path) // pid_t = int
{
    using namespace std;

    // fork into two programs , use pid to cofirm , spilt into parent and child
    // return pid is parent's pid
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

    // this is parnet program
    else
    {
        int status;
        // aprent status , wait for child status change
        waitpid(pid, &status, 0);

        // child asked to be traced , when child starts , kernel makes it pause
        // if child stops , can debug , return child pid
        if (WIFSTOPPED(status))
        {
            return pid; // child pid
        }
        else
        {
            return -1;
        }
    }
}

void clean_child_program(pid_t child_pid)
{
    using namespace std;

    // end child program
    ptrace(PTRACE_KILL, child_pid, nullptr, nullptr);
    int status;

    // let parent read child's status(report after end)
    waitpid(child_pid, &status, 0);
    cout << "child_program_end" << endl;
    return;
}