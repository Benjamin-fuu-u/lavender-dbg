#pragma once

#include <string>
#include <vector>
#include <sys/types.h>
using namespace std;

struct MemoryRegion{    //save the data and mark the data
    string start_address;   //memory start address
    string end_address;     //memory end address
    string permissions;     //read or write or run or private
    string offset;          //where to move in the harddisk
    string device;          //which harddisk
    string inode;           //the program number
    string pathname;        //the path
    
    bool is_readable = false;   //can read or not
    bool is_writeable = false;  //can write or not
    bool is_executable = false; //can run or not
    bool is_private = false;    //private or not private
 };

enum class RegionType{  //classification the data
    EXECUTABLE_CODE , //the code can run
    LIBC ,             //C function
    STACK ,          
    HEAP , 
    WRITEABLE_DATA , //the data can write
    READ_ONLY , 
    VDSO ,          
    // something the program should know  , it can reduce the time that the program ask the computer
    OTHER
};

vector<MemoryRegion> read_memory_maps(pid_t pid);
MemoryRegion parse_map_line(const string& line);    //detail memory struct
RegionType classify_region(const MemoryRegion& region);     //give each a classfication
void print_colored_maps(const vector<MemoryRegion>& regions);
