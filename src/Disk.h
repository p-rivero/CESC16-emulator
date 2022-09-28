#pragma once

#include "Globals.h"
#include "Memory.h"

#include <string>
#include <fstream>


class DiskController {
private:
    // Communication with CPU
    volatile word *input;
    volatile word *output;
    
    std::string currentFile = ""; // 8.3 filename (8 char long name + 3 char long extension)
    int currentDepth = 0;
    bool file_is_open = false;
    std::fstream file; // Actual file used for IO
    byte buf[0x10000];
    
    word read();
    void write(word data);
    void clear();
    
    void expectAck();
    int readByteStream(byte *buffer, int size);
    void writeByteStream(const byte *buffer, int length);
    std::string readString();
    void writeString(const std::string &str);
    
    inline void checkFileIsOpen(std::string funct);
    inline void checkSetFileName(std::string funct);
    
    void setFileName();
    void openFile();
    void closeFile();
    void deleteFile();
    void readFile();
    void writeFile();
    void moveFileCursor();
    void getFileCursor();
    void listDir();
    void cd();
    void mkdir();
    void getInfo();
    
    
public:
    DiskController(volatile word *input_reg, volatile word *output_reg, const std::string& root_directory);
    void main_loop();
};



class Disk : public MemCell {
private:
    volatile word input_reg;
    volatile word output_reg;
    
public:
    static const int BUSY_BIT = 1 << 9;
    
    static const int ACK = 0x100;
    static const int ERROR = 0x101;
    
    static const int CMD_setFileName = 0x110;
    static const int CMD_openFile = 0x111;
    static const int CMD_closeFile = 0x112;
    static const int CMD_deleteFile = 0x113;
    static const int CMD_readFile = 0x114;
    static const int CMD_writeFile = 0x115;
    static const int CMD_moveFileCursor = 0x116;
    static const int CMD_getFileCursor = 0x117;
    static const int CMD_listDir = 0x118;
    static const int CMD_cd = 0x119;
    static const int CMD_mkdir = 0x11A;
    static const int CMD_getInfo = 0x11B;
    
    Disk();
    
    // WRITE
    virtual MemCell& operator=(word rhs);
    // READ
    virtual operator int() const;
    
};
