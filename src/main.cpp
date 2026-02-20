#include<iostream>
#include<cstdlib>
#include<vector>

#include"process/process.h"
#include"memory/memory.h"
#include"common/color.h"

using namespace std;

int main(int argc , char*argv[])
{
    using namespace std;
    if(argc != 2){
        print_how_to_use(argv[0]);  //argv[0] = program name
        exit(1);
    }
    const char* target_path = argv[1];
    pid_t child_pid = start_child_and_father_program(target_path);
    if(child_pid == -1){
        cerr<<"Error"<<endl;
        exit(1);
    }

    vector<MemoryRegion> regions = read_memory_maps(child_pid);

    print_colored_maps(regions);

    clean_child_program(child_pid);
    return 0;
}

