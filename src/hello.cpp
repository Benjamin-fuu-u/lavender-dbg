#include<bits/stdc++.h>
#include<iostream>
#include<unistd.h> // for get pid 

using namespace std;

int main()
{
    cout<<"hello, word"<<endl;
    cout<<"My pid"<<getpid()<<endl;     //pid is the unique number for each program
    cout<<"Now stop . You can get pid and see the mapview ."<<endl;
    cin.get();  //pause the program 
    return 0;
}
