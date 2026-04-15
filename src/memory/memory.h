#pragma once

#include <string>
#include <vector>
#include <sys/types.h>
#include <cstdint>

using namespace std;

// Data structure to store maps info
struct MemoryRegion
{
    string start_address;
    string end_address;
    string permissions; // Permissions of this section
    string offset;      // From which bit to move
    string device;      // Which hard disk it exists
    string inode;       // File number
    string pathname;

    // Permissions
    bool is_readable = false;
    bool is_writeable = false;
    bool is_executable = false;
    bool is_private = false;
};

enum class RegionType
{
    // Classify each section
    EXECUTABLE_CODE,
    LIBC,
    STACK,
    HEAP,
    WRITEABLE_DATA,
    READ_ONLY,
    VDSO,
    OTHER
};

// Get maps data
vector<MemoryRegion> read_memory_maps(pid_t pid);

// Convert plain text to object above
MemoryRegion parse_map_line(const string &line);

// Classify which type of region
RegionType classify_region(const MemoryRegion &region);

// Print colored maps
void print_colored_maps(const vector<MemoryRegion> &regions);

// Get base address (ASLR)
uint64_t get_base_address(const vector<MemoryRegion> &regions, const string &target_path);