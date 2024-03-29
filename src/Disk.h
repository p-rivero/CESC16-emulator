#pragma once

#include "Globals.h"
#include "Memory.h"

#include <string>
#include <fstream>
#include <atomic>


class DiskController {
private:
    // Communication with CPU
    std::atomic<word> * const input;
    std::atomic<word> * const output;
    
    std::string currentFile = ""; // 8.3 filename (8 char long name + 3 char long extension)
    bool file_is_open = false;
    std::fstream file; // Actual file used for IO
    using buf_t = std::array<byte, 0x10000>;
    buf_t io_buffer; // Buffer for file IO
    
    word read() const;
    void write(word data);
    void clear();
    
    void expectAck() const;
    size_t readByteStream(buf_t& buffer) const;
    void writeByteStream(const buf_t& buffer, size_t length);
    std::string readString();
    void writeString(const std::string &str);
    
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
    DiskController(std::atomic<word> *input_reg, std::atomic<word> *output_reg, const std::string& root_directory);
    [[noreturn]] void main_loop();
};



class Disk : public MemCell {
private:
    std::atomic<word> input_reg = 0;
    std::atomic<word> output_reg = 0;
    
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
