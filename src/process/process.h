// ensure only one copy of process
#pragma once

#include <sys/types.h>

// show how to start this program
void print_how_to_use(const char *program_name);

// start child program pid_t to store PID
pid_t start_child_and_father_program(const char *target_path);

// end child program
void clean_child_program(pid_t child_pid);