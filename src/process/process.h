#pragma once

#include<sys/types.h>   //data ensure 

void print_how_to_use(const char* program_name);
pid_t start_child_and_father_program(const char*target_path);  //pid_t = int
void clean_child_program(pid_t child_pid);
