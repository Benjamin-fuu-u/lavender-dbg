#include<iostream>
#include<fstream>  //file stream
#include<string>
#include<cstdlib>   // provide exit to stop program and see the mapview
#include<cerrno>    //show what happend when open the file " errno "
#include<cstring>   //use strerror "strerror"

using namespace std;

int main(int argc , char*argv[])
{
    if(argc != 2){
        // cerr : the different way for direct output
        cerr<<"error "<<argv[0]<<" need the target ID"<<endl;
        cerr<<"For example "<<argv[0]<<" 1234";
        exit(1);    //return error 1
    }

    
    string id = argv[1] , path = "/proc/" + id + "/maps";
    // the "path" store the detail about the target project

    cout<<"reading : "<<path<<endl;
    cout<<"========================="<<endl;
    cout<<endl;

    // "c_str" change string to char
    ifstream file(path.c_str());    //open file 
    
    if(!file){  //!file means canʼt opem the file
        cerr<<"Error : "<<strerror(errno)<<" pid = "<<id<<endl;
        exit(1);
    }

    cout<<"Memory Map Mapping"<<endl;
    cout<<endl;
    string mapping;

    while(getline(file , mapping)){
        cout<<mapping<<endl;
    }

    file.close();   //close the file

    cout<<"End"<<endl;
    cout<<"========================="<<endl;

    return 0;
}
