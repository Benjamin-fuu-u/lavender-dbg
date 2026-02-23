#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint> //uint8_t , uint_16t  ensure the int size
#include <cstring> //"memcmp" move or compare the binary

#include "elf_view.h"
#include "../common/color.h"

#include <elf.h> //the header file only can use in linux to view the elf

// static only the code can use the function
static const char *elf_type_to_string(uint16_t e_type)
{
    if (e_type == 0)
        return "ET_NONE (Unknown)"; // the elf may broken
    if (e_type == 1)
        return "ET_REL (Relocatable  .o)"; // machine code , global variable , symbol table (what function in the code) , Relocation(there is a function but unknown the address)
    if (e_type == 2)
        return "ET_EXEC (Executable)"; // the code can run (the address is stable)
    if (e_type == 3)
        return "ET_DYN (shared or PIE)"; // shared function(.so) or the code can be run (the address will change each build)
    if (e_type == 4)
        return "ET_CORE (Core dump)"; // take the picture when the code crush
    else
        return "ET_* (other)";
}

static const char *elf_machine_to_string(uint16_t e_machine)
{
    if (e_machine == 3)
        return "x86 (EM_386)"; // old 32
    if (e_machine == 62)
        return "x-86-64 (EM_X*^_64)"; // new 64
    if (e_machine == 40)
        return "ARM (EM_ARM)"; // old phone
    if (e_machine == 183)
        return "AArch64 (EM_AARCH64)"; // iphone or android
    if (e_machine == 243)
        return "RISC-V (EM_RISCV)"; // new RISC-V
    else
        return "Other";
}

static const char *elf_data_to_string(uint8_t ei_data)
{
    if (ei_data == ELFDATA2LSB)
        return "Little Endian (ELFDATA2LSB)";
    if (ei_data == ELFDATA2MSB)
        return "Big Endian (ELFDATA2MSB)";
    else
        return "Unknown";
}

static const char *elf_class_to_string(uint8_t ei_class)
{
    if (ei_class == ELFCLASS32)
        return "32-bit (ELFCLASS32)"; // 32bit
    if (ei_class == ELFCLASS64)
        return "64-bit (ELFCLASS64)"; // 64bit
    else
        return "unknown";
}

template <typename Ehdrt> // if "Elf64" Ehdrt is 64 byte , if "Elf32" Ehdrt is 32 byte

static void print_ehdr_common(const Ehdrt &ehdr)
{
    cout << "Type: " << Color::BOLD_LIME_GREEN << elf_type_to_string(ehdr.e_type) << Color::RESET << endl; // ehdr.e_type , ehdr.e_machine , ehdr.e_entry is a part of struct
    cout << "Machine: " << Color::BOLD_LIME_GREEN << elf_machine_to_string(ehdr.e_machine) << Color::RESET << endl;
    cout << "Entry: " << Color::BOLD_LIME_GREEN << hex << ehdr.e_entry << Color::RESET << dec << endl;
    //"hex" turn to hex code(16) , "dec" turn to decimal (10) , "e_entry" the start (_start) address of the program
    cout << endl;
    // where the code should move (used by operating system)
    cout << "PHoff: " << Color::BOLD_LIME_GREEN << "0x" << hex << ehdr.e_phoff << Color::RESET << dec << endl; // where the list start
    cout << "Phentsz: " << Color::BOLD_LIME_GREEN << ehdr.e_phentsize << Color::RESET << endl;                 // each line wide
    cout << "PHnum: " << Color::BOLD_LIME_GREEN << ehdr.e_phnum << Color::RESET << endl;                       // how many line

    cout << endl;

    cout << "SHoff: " << Color::BOLD_LIME_GREEN << "0x" << hex << ehdr.e_shoff << Color::RESET << dec << endl; // SH start address
    cout << "SHentsz: " << Color::BOLD_LIME_GREEN << ehdr.e_shentsize << Color::RESET << endl;                 // each line wide
    cout << "SHnum: " << Color::BOLD_LIME_GREEN << ehdr.e_shnum << Color::RESET << endl;                       // how many line
    cout << "SHstrndx: " << Color::BOLD_LIME_GREEN << ehdr.e_shstrndx << Color::RESET << endl;                 // the address point to the name box
}

bool print_elf_info(const string &target_path)
{
    using namespace std;

    ifstream file(target_path, ios::binary); // use binary to open the file
    if (!file)
    {
        cerr << "[ELF] Error " << strerror(errno) << endl;
        return false;
    }

    // read the first 16 bytes
    unsigned char first_info[EI_NIDENT]; //"EI_NIDENT" means 16 bytes
    file.read(reinterpret_cast<char *>(first_info), EI_NIDENT);
    //"reinterpret" change unsigned char to char* , "read" and "EI_NIDENT" get 16 bytes from file

    if (!file || file.gcount() != EI_NIDENT)
    {
        //"gcount" how many bytes has been getten
        cerr << "[ELF] Error the file too short" << endl;
        return false;
    }

    // compare the memory  , compare first_info and ELFMAG(means 0x7f , "E" , "L" , "F") , the length is SELFMAG(means 4)
    if (memcmp(first_info, ELFMAG, SELFMAG) != 0)
    {
        cerr << "[ELF] Error nit an ELF" << endl;
        return false;
    }

    cout << "ELF info" << endl;
    cout << "path: " << Color::BOLD_LIME_GREEN << target_path << Color::RESET << endl;

    cout << "Class: " << Color::BOLD_LIME_GREEN << elf_class_to_string(first_info[EI_CLASS]) << Color::RESET << endl;         // ELF_CLASS = 4 32 or 64  bytes
    cout << "Endian: " << Color::BOLD_LIME_GREEN << elf_data_to_string(first_info[EI_DATA]) << Color::RESET << endl;          // memory to 1 (low) or 2(high)
    cout << "OSABI " << Color::BOLD_LIME_GREEN << static_cast<int>(first_info[EI_OSABI]) << Color::RESET << " (raw)" << endl; // show the system (like linux or windows) it not very important in the moder day
    // use static to change to int or the code will build it as an ASCII
    // EI_OSABI = 7  EI_VERSION = 6 means the ELF version
    // raw means the origin data

    file.clear();            // clear EOF (end) or if there is a fail flag , start read from beanging
    file.seekg(0, ios::beg); // start from 0 , ios::beg means the start

    if (first_info[EI_CLASS] == ELFCLASS64)
    {                      // 64 byte
        Elf64_Ehdr ehdr{}; // a struct store the beinging ELF information , {} means itʼs none value avoid trash value
        file.read(reinterpret_cast<char *>(&ehdr), sizeof(ehdr));
        // read(store the information to &ehdr , read 64 bytes)

        if (!file)
        {
            cerr << "[ELF] Errror: " << strerror(errno) << endl;
            return false;
        }

        print_ehdr_common(ehdr);
    }
    else if (first_info[EI_CLASS] == ELFCLASS32)
    {
        Elf32_Ehdr ehdr{};

        file.read(reinterpret_cast<char *>(&ehdr), sizeof(ehdr));

        if (!file)
        {
            cerr << "[ELF] Error: " << strerror(errno) << endl;
            return false;
        }
        print_ehdr_common(ehdr);
    }
    else
    {
        cerr << "[ELF] Error: unknown ELF class" << endl;
        return false;
    }
    cout << endl;
    return true;
}
