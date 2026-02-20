#include<iostream>
#include<fstream>   //file stream
#include<string>    
#include<sstream>       //string stream
#include<iomanip>       //ctrl output input 
#include<cerrno>    //"errno" show the error
#include<cstring>   //"strerror" translate the error 

#include"memory.h"
#include"../common/color.h"

using namespace std;

MemoryRegion parse_map_line(const string& line)     //detail memory struct
{
    using namespace std;

    MemoryRegion region;
    istringstream iss(line);  // istringstream cam spilt the input  ">>" spilt input whem " "

    //read memory range

    string addr_range;
    iss >> addr_range;

    size_t dash_pos = addr_range.find("-"); // start and emd connect by an "-" find it pos

    if(dash_pos != string::npos){          // ensure the pos is possible "npos" = canʼt find
        region.start_address = addr_range.substr(0 , dash_pos); // 0 to dash_pos
        region.end_address = addr_range.substr(dash_pos + 1);   //dash_pos + 1 to end 
    }

    iss >> region.permissions;

    if(region.permissions.length() >= 4){
        region.is_readable = (region.permissions[0] == 'r');
        region.is_writeable = (region.permissions[1] == 'w');
        region.is_executable = (region.permissions[2] == 'x');
        region.is_private = (region.permissions[3] == 'p');
    }

    iss >> region.offset;
    iss >> region.device;
    iss >> region.inode;

    getline(iss , region.pathname);

    size_t start = region.pathname.find_first_not_of(" \t");
    if(start != string::npos){
        region.pathname = region.pathname.substr(start);
    }
    else{
        region.pathname = "";
    }

    return region;
}

vector<MemoryRegion> read_memory_maps(pid_t pid)
{
    using namespace std;
    vector<MemoryRegion> regions;
    string path = "/proc/" + to_string(pid) + "/maps";

    ifstream file(path);

    if(!file){
        cerr<<"Error "<<strerror(errno)<<endl;
        return regions;
    }

    string line;
    while(getline(file , line)){
        MemoryRegion region = parse_map_line(line);
        regions.push_back(region);
    }
    file.close();
    return regions;
}

RegionType classify_region(const MemoryRegion& region)     //give each a classfication
{
    using namespace std;

    const string& path = region.pathname;

    if(path == "[stack]"){
        return RegionType::STACK;
    }
    else if (path == "[heap]"){
        return RegionType::HEAP;
    }
    else if(path == "[vdso]"){
        return RegionType::VDSO;
    }

    if(path.find("libc") != string::npos){
        if(region.is_executable){
            return RegionType::LIBC;
        }
    }

    else if(region.is_executable){
        return RegionType::EXECUTABLE_CODE;
    }

    else if(region.is_readable && !region.is_writeable && !region.is_executable){
        return RegionType::READ_ONLY;
    }
    return RegionType::OTHER;
}

void print_colored_maps(const vector<MemoryRegion>& regions)      
{
    using namespace std;

    cout<<Color::BOLD_WHITE<<"Address_range                             "<<"perm    "<<"offset      "<<"LABEL      "<<"path"<<endl;

    for(const auto& region : regions){
        RegionType type = classify_region(region); 

        

        cout<<Color::BOLD_CYAN<<left<<setw(18)<<region.start_address<<" - "<<left<<setw(18)<<region.end_address;
        
        cout<<"   ";
        
        cout<<Color::RESET;
        if(region.is_readable){
            cout<<"r";
        }
        else {
            cout<<"-";
        }

        cout<<Color::RESET;
        if(region.is_writeable){
            cout<<Color::MAGENTA<<"w";
        }
        else{
            cout<<"-";
        }

        cout<<Color::RESET;
        if(region.is_executable){
            cout<<Color::CYAN<<"x";
        }
        else{
            cout<<"-";
        }

        cout<<Color::RESET;
        if(region.is_private){
            cout<<"p";
        }
        else {
            cout<<"s"<<endl;
        }

        cout<<Color::RESET;
        cout<<Color::YELLOW<<"    "<<setw(10)<<region.offset<<"  ";

        cout<<Color::RESET;

        if(type == RegionType::EXECUTABLE_CODE){
            cout<<Color::BOLD_LIME_GREEN<<"[CODE]   ";
        }
        else if(type == RegionType::LIBC){
            cout<<Color::BOLD_ORANGE<<"[LIBC]   ";
        }
        else if(type == RegionType::STACK){
            cout<<Color::BOLD_CORAL_RED<<"[STACK]   ";
        }
        else if(type == RegionType::HEAP){
            cout<<Color::BOLD_SKY_BLUE<<"[HEAP]   ";
        }
        else if(type == RegionType::WRITEABLE_DATA){
            cout<<Color::BOLD_LAVENDER<<"[DATA]   ";
        }
        else if(type == RegionType::VDSO){
            cout<<Color::BOLD_CYAN<<"[VDSO]   ";
        }
        else{
            cout<<Color::BOLD_GREY<<"[OTHER]   ";
        }

        cout<<Color::RESET<<"   ";

        cout<<region.pathname<<endl;
    }


}    
