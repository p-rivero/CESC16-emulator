#pragma once

#include "Globals.h"
#include "Memory.h"

#include <string>

class DiskController {
private:
    volatile word *input;
    volatile word *output;
    std::string currentFile = ""; // 8.3 filename (8 char long name + 3 char long extension)
    std::string currentDir;
    bool file_is_open = false;
    
    word read() const;
    void write(word data);
    void clear();
    
    void expectAck();
    void writeByteStream(byte *buffer, int length);
    void readByteStream(byte *buffer, int length);
    
    inline void checkFileIsOpen(const std::string& funct) const;
    inline void checkSetFileName(const std::string& funct) const;
    
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
    DiskController(volatile word *input_reg, volatile word *output_reg);
    void main_loop();
};



class Disk : public MemCell {
private:
    volatile word input_reg = 0;
    volatile word output_reg = 0;
    
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
    MemCell& operator=(word rhs) override;
    // READ
    operator word() const override;
    
};
