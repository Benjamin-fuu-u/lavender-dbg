#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <cerrno>
#include <cstring>
#include <cstdint>

#include "memory.h"
#include "../common/color.h"

using namespace std;

MemoryRegion parse_map_line(const string &line)
{
    using namespace std;

    MemoryRegion region;
    istringstream iss(line);

    string addr_range;
    iss >> addr_range;

    // Two memory addresses separated by " - "
    size_t dash_pos = addr_range.find("-");

    // Split memory address by " - " into start and end address, npos means not found
    if (dash_pos != string::npos)
    {
        region.start_address = addr_range.substr(0, dash_pos);
        region.end_address = addr_range.substr(dash_pos + 1);
    }

    iss >> region.permissions;

    if (region.permissions.length() >= 4)
    {
        // Store permissions
        region.is_readable = (region.permissions[0] == 'r');
        region.is_writeable = (region.permissions[1] == 'w');
        region.is_executable = (region.permissions[2] == 'x');
        region.is_private = (region.permissions[3] == 'p');
    }

    iss >> region.offset;
    iss >> region.device;
    iss >> region.inode;

    getline(iss, region.pathname);

    // Find first char that is not tab or space from start, remove spaces before path
    size_t start = region.pathname.find_first_not_of(" \t");
    if (start != string::npos)
    {
        region.pathname = region.pathname.substr(start);
    }
    else
    {
        region.pathname = "";
    }

    return region;
}

vector<MemoryRegion> read_memory_maps(pid_t pid)
{
    using namespace std;
    vector<MemoryRegion> regions;

    // Linux stores maps data in this path
    string path = "/proc/" + to_string(pid) + "/maps";

    // Open file stream for input
    ifstream file(path);

    if (!file)
    {
        cerr << "Error " << strerror(errno) << endl;
        return regions;
    }

    // Classify for each line
    string line;
    while (getline(file, line))
    {
        MemoryRegion region = parse_map_line(line);
        regions.push_back(region);
    }

    // Close file after opening
    file.close();
    return regions;
}

RegionType classify_region(const MemoryRegion &region)
{
    using namespace std;

    const string &path = region.pathname;

    // Judge what kind of memory section from file path
    if (path == "[stack]")
    {
        return RegionType::STACK;
    }
    else if (path == "[heap]")
    {
        return RegionType::HEAP;
    }
    else if (path == "[vdso]")
    {
        return RegionType::VDSO;
    }

    // Is libc and executable
    if (path.find("libc") != string::npos)
    {
        if (region.is_executable)
        {
            return RegionType::LIBC;
        }
    }
    // No libc
    else if (region.is_executable)
    {
        return RegionType::EXECUTABLE_CODE;
    }
    else if (region.is_readable && region.is_writeable)
    {
        return RegionType::WRITEABLE_DATA;
    }
    return RegionType::OTHER;
}

void print_colored_maps(const vector<MemoryRegion> &regions)
{
    using namespace std;

    cout << Color::BOLD_LAVENDER
         << "=== [ " << Color::YELLOW << "Memory Mapping" << Color::BOLD_LAVENDER
         << " ] ====================================================================================="
         << Color::RESET << endl;

    cout << Color::BOLD_WHITE << "Address_range                         " << "perm    " << "offset      " << "LABEL      " << "path" << endl;

    for (const auto &region : regions)
    {
        // Classify each cut maps data
        RegionType type = classify_region(region);

        // Memory section
        cout << Color::BOLD_DARK_BLUE << " " << left << setw(16) << region.start_address << " - " << left << setw(16) << region.end_address;

        cout << "   ";

        cout << Color::RESET;

        // Permissions
        if (region.is_readable)
        {
            cout << "r";
        }
        else
        {
            cout << "-";
        }

        cout << Color::RESET;
        if (region.is_writeable)
        {
            cout << Color::MAGENTA << "w";
        }
        else
        {
            cout << "-";
        }

        cout << Color::RESET;
        if (region.is_executable)
        {
            cout << Color::BOLD_DARK_BLUE << "x";
        }
        else
        {
            cout << "-";
        }

        cout << Color::RESET;
        if (region.is_private)
        {
            cout << "p";
        }
        else
        {
            cout << "s" << endl;
        }

        cout << Color::RESET;
        cout << Color::YELLOW << "    " << setw(10) << region.offset << "  ";

        cout << Color::RESET;

        // Show section type, let user find important sections at a glance

        if (type == RegionType::EXECUTABLE_CODE)
        {
            cout << Color::BOLD_LIME_GREEN << "[CODE]   ";
        }
        else if (type == RegionType::LIBC)
        {
            cout << Color::BOLD_ORANGE << "[LIBC]   ";
        }
        else if (type == RegionType::STACK)
        {
            cout << Color::BOLD_CORAL_RED << "[STACK]   ";
        }
        else if (type == RegionType::HEAP)
        {
            cout << Color::BOLD_ORANGE << "[HEAP]   ";
        }
        else if (type == RegionType::WRITEABLE_DATA)
        {
            cout << Color::BOLD_LAVENDER << "[DATA]   ";
        }
        else if (type == RegionType::VDSO)
        {
            cout << Color::BOLD_DARK_BLUE << "[VDSO]   ";
        }
        else
        {
            cout << Color::BOLD_GREY << "[OTHER]   ";
        }

        cout << region.pathname << endl;
    }
}

uint64_t get_base_address(const vector<MemoryRegion> &regions, const string &target_path)
{
    // Absolute path
    string filename = target_path;

    // Cut out the last /
    size_t pos = target_path.rfind('/');

    if (pos != string::npos)
        filename = target_path.substr(pos + 1);

    // The first one encountered is the program's load header position, which is base_address
    for (const auto &region : regions)
    {
        if (region.pathname.find(filename) != string ::npos)
            return stoull(region.start_address, nullptr, 16);
    }

    return 0;
}