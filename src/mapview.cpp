#include<iostream>
#include<fstream>   //file stream
#include<string>
#include<cstdlib>   //exit
#include<cerrno>    //"errno" show the error
#include<cstring>   //"strerror" translate the error 
#include<unistd.h>  //fork , execv , pid_t
#include<sys/ptrace.h>  //ptrace , PTRACE_TRACEME , ptrace_CONT , PTRACE_KILL
#include<sys/wait.h>    //waitpid , WIFISTOPPED
#include<sstream>       //string stream
#include<iomanip>       //ctrl output input
#include<iomanip> 
#include<vector> 

using namespace std;

// namespace : only need to type "Color : RED" means \033[0m    
namespace Color{     //define colors
    const char* RESET = "\033[0m";  //default color
    const char* RED = "\033[91m";    //red
    const char* GREEN =  "\033[92m"; //green
    const char* YELLOW = "\033[93m"; //yellow
    const char* BLUE = "\033[94m";   //blue
    const char* MAGENTA = "\033[38;2;255;0;255m";  //magenta
    const char* CYAN = "\033[96m";  //cyan
    const char* WHITE = "\033[97m"; //white

    //bold text
    const char* BOLD_CORAL_RED = "\033[1;38;2;255;107;107m";
    const char* BOLD_LIME_GREEN = "\033[1;38;2;0;200;83m";
    const char* BOLD_ORANGE = "\033[1;38;2;255;167;38m";
    const char* BOLD_SKY_BLUE = "\033[1;38;2;100;181;246m";
    const char* BOLD_LAVENDER = "\033[1;38;2;186;104;200m";
    const char* BOLD_CYAN = "\033[1;38;2;100;181;246m";
    const char* BOLD_GREY = "\033[1;38;2;158;158;158m";
    const char* BOLD_WHITE = "\033[1;97m";
 }

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

void print_how_to_use(const char* program_name)
{
    using namespace std;
    cerr<<"Error "<<program_name<<" need the target path"<<endl;
    cerr<<"For example : "<<program_name<<" ./hello"<<endl;
    return ;
}

pid_t start_child_and_father_program(const char*target_path)  //pid_t = int
{
    using namespace std;
    pid_t pid = fork(); //it will make two program 
                        //one is the child program it pid is 0
                        //the other is the father program it pid is not 0
    
    if(pid == -1){  //pid = -1 means error
        cerr<<"Fork error"<<strerror(errno)<<endl;
        return -1;
    }

    else if(pid == 0){      //child program
        ptrace(PTRACE_TRACEME , 0 , nullptr , nullptr);   // "PTRACE_TRACEME" let the father program trace
        char*args[] = {const_cast<char*>(target_path) , nullptr};  //const_cast change "const char" to "char" tempory
                                                                   //args[0] = target path , args[1] = nullptr means the args end 
        execv(target_path , args);  //jump to target program   "args" will be placed in the main(args[0]) 

        //if failed the following will run
        cerr<<"Errno : "<<strerror(errno)<<endl;
        exit(1);
    }

    else{   //itʼs father program pid is the child id
        int status;
        waitpid(pid , &status , 0); //waitpid will wait until the child program pause
                                    //"0" means the function will return when the child program pause or end
                                    // & will convey the location so the function can modift the value
        if(WIFSTOPPED(status)){
                return pid; //child pid
        }
        else{
            return -1;
        }
    }
}

void clean_child_program(pid_t child_pid)
{
    using namespace std;
    ptrace(PTRACE_KILL ,child_pid , nullptr , nullptr);
    //kill the child program
    int status;
    waitpid(child_pid , &status , 0);
    cout<<"child_program_end"<<endl;
    return ;
}

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
