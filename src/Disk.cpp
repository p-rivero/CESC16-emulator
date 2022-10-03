#include "Disk.h"
#include "Exceptions/DiskControllerException.h"
#include "Utilities/Assert.h"
#include "Utilities/ExitHelper.h"

#include <thread>
#include <filesystem>
#include <unistd.h> // chdir
#include <cstring> // strerror


std::string Globals::disk_root_dir = "";

// DISK CONTROLLER

DiskController::DiskController(volatile word *input_reg, volatile word *output_reg, const std::string& root_directory) : input(input_reg), output(output_reg) {
    assert(input_reg != nullptr);
    assert(output_reg != nullptr);
    
    // Change working directory to the new root
    if (root_directory != "" && chdir(root_directory.c_str()) != 0) {
        throw DiskControllerException("Failed to change root directory to " + root_directory);
    }
}

[[noreturn]] void DiskController::main_loop() {
    while (true) {
        switch (word cmd = read(); cmd) {
            case Disk::CMD_setFileName: setFileName(); break;
            case Disk::CMD_openFile: openFile(); break;
            case Disk::CMD_closeFile: closeFile(); break;
            case Disk::CMD_deleteFile: deleteFile(); break;
            case Disk::CMD_readFile: readFile(); break;
            case Disk::CMD_writeFile: writeFile(); break;
            case Disk::CMD_moveFileCursor: moveFileCursor(); break;
            case Disk::CMD_getFileCursor: getFileCursor(); break;
            case Disk::CMD_listDir: listDir(); break;
            case Disk::CMD_cd: cd(); break;
            case Disk::CMD_mkdir: mkdir(); break;
            case Disk::CMD_getInfo: getInfo(); break;
        
            case Disk::ACK: throw DiskControllerException("Unexpected ACK instead of command");
            default: throw DiskControllerException("Unrecognized command");
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        clear(); // Command has been processed: clear the busy bit
    }
}

// Read the input register
word DiskController::read() const {
    word data = 0;
    // Poll busy bit until an input is detected
    while ((data & Disk::BUSY_BIT) == 0) {
        // Acquire exit lock to prevent segfault when the main thread is exiting
        std::scoped_lock<std::mutex> lock(ExitHelper::exit_mutex);
        
        data = *input;
    }
    assert(data <= 0x3FF);
    
    // Don't return the busy bit
    return data & 0x1FF;
}

// Write to the output register
void DiskController::write(word data) {
    assert(data <= 0x1FF);
    {
        // Acquire exit lock to prevent segfault when the main thread is exiting
        std::scoped_lock<std::mutex> lock(ExitHelper::exit_mutex);
        *output = data;
    }
    expectAck();
}

// Clear the input register
void DiskController::clear() {
    // Acquire exit lock to prevent segfault when the main thread is exiting
    std::scoped_lock<std::mutex> lock(ExitHelper::exit_mutex);
    
    *input = 0;
}


// Helper functions

// The CPU should send an Ack next
void DiskController::expectAck() const {
    word in = read();
    if (in != Disk::ACK)
        throw DiskControllerException("Expected an ACK");
}

// Returns the number of read bytes
size_t DiskController::readByteStream(buf_t& buffer) const {
    for (size_t i = 0; i < buffer.size(); i++) {
        word data = read();
        // If an ACK is received, the stream is over
        if (data == Disk::ACK) return i;
        buffer[i] = byte(data);
    }
    throw DiskControllerException("DiskController: Buffer overflow");
}

// Write n byes from buffer
void DiskController::writeByteStream(const buf_t& buffer, size_t n) {
    for (int i = 0; i < n; i++) {
        write(buffer[i]);
    }
    // Indicate the end of the message
    write(Disk::ACK);
}

std::string DiskController::readString() {
    size_t n = readByteStream(io_buffer);
    for (size_t i = 0; i < n; i++) {
        if (io_buffer[i] >= 0x80) throw DiskControllerException("Non-ascii character received: " + std::to_string(io_buffer[i]));
    }
    return std::string((char*) io_buffer.begin(), n);
}

void DiskController::writeString(const std::string &str) {
    // Copy the string to the IO buffer
    for (int i = 0; i < str.length(); i++) {
        if (str[i] >= 0x80) throw DiskControllerException("Non-ascii character in string: " + str);
        io_buffer[i] = str[i];
    }
    writeByteStream(io_buffer, str.length());
}

void DiskController::checkFileIsOpen(const std::string& funct) const {
    if (!file_is_open)
        throw DiskControllerException(funct + " was called but no file was open");
}
void DiskController::checkSetFileName(const std::string& funct) const {
    if (currentFile.empty())
        throw DiskControllerException(funct + " was called without using setFileName first");
}



// Disk operations

// Read the value of currentFile
void DiskController::setFileName() {
    currentFile = readString();
    write(Disk::ACK);
}

void DiskController::openFile() {
    checkSetFileName("openFile");
    
    file.open(currentFile, std::fstream::binary);
    file_is_open = true;
}

void DiskController::closeFile() {
    checkFileIsOpen("closeFile");
    
    file.close();
    file_is_open = false;
}

void DiskController::deleteFile() {
    checkSetFileName("deleteFile");
    
    if (file_is_open) closeFile();
    std::remove(currentFile.c_str());
}

void DiskController::readFile() {
    // Called readRaw() on CH376 library
    checkFileIsOpen("readFile");
    
    // Read 2-byte size (little endian)
    unsigned int size = read();
    assert(size <= 0xFF);
    size |= read() << 8;
    assert(size <= 0xFFFF);
    
    expectAck();
    
    // Read from file
    file.read((char*)io_buffer.begin(), size);
    // Update the write cursor
    std::streampos read_pos = file.tellg();
    file.seekp(read_pos);
    
    // Send to CPU
    writeByteStream(io_buffer, size);
}

void DiskController::writeFile() {
    checkFileIsOpen("writeFile");
    
    // Get data
    size_t n = readByteStream(io_buffer);
    // Write data
    file.write((char*)io_buffer.begin(), n);
    // Update the read cursor
    std::streampos write_pos = file.tellp();
    file.seekg(write_pos);
    // Send ACK to CPU
    write(Disk::ACK);
}

void DiskController::moveFileCursor() {
    checkFileIsOpen("moveFileCursor");
    
    // Read 4-byte position (little endian)
    unsigned long long int pos = read();
    assert(pos <= 0xFF);
    pos |= read() << 8;
    assert(pos <= 0xFFFF);
    pos |= read() << 16;
    assert(pos <= 0xFFFFFF);
    pos |= read() << 24;
    assert(pos <= 0xFFFFFFFF);
        
    expectAck();
    
    // Move cursor
    file.seekp(pos);
    file.seekg(pos);
    // Send ACK to CPU
    write(Disk::ACK);
}

void DiskController::getFileCursor() {
    checkFileIsOpen("getFileCursor");
    
    unsigned long long int read_pos = file.tellg();
    unsigned long long int write_pos = file.tellp();
    assert(read_pos <= 0xFFFFFFFF);
    assert(read_pos == write_pos);
    
    // Send 4-byte position (little endian)
    write(read_pos & 0xFF);
    write((read_pos >> 8) & 0xFF);
    write((read_pos >> 16) & 0xFF);
    write((read_pos >> 24) & 0xFF);
    // Send ACK to CPU
    write(Disk::ACK);
}

void DiskController::listDir() {
    std::string result = "";
    for (const auto &entry : std::filesystem::directory_iterator(".")) {
        result += entry.path().filename().string() + "\n";
    }
    
    // Send result and ACK to CPU
    writeString(result);
}

void DiskController::cd() {
    std::string dir = readString();
    // TODO: Check if dir is valid
    if (chdir(dir.c_str()) != 0) {
        throw DiskControllerException("Failed to change working directory to " + dir + ": " + strerror(errno));
    }
    
    // Send ACK to CPU
    write(Disk::ACK);
}

void DiskController::mkdir() {
    std::string dir = readString();
    // TODO: check if dir is valid
    std::filesystem::create_directory(dir);
    
    // Send ACK to CPU
    write(Disk::ACK);
}

void DiskController::getInfo() {
    // Dummy text for emulator
    std::string info = "";
    info += "USB device OK (v.67) - EMULATED\n";
    info += "Total sectors: 10000\n";
    info += "Free sectors: 1234\n";
    info += "File system: FAT32\n";
    
    // Send result and ACK to CPU
    writeString(info);
}



// DISK PERIPHERAL

Disk::Disk() {
    std::thread([this]() {
        try {
            DiskController controller(&input_reg, &output_reg, Globals::disk_root_dir);
            controller.main_loop();
        }
        catch (const DiskControllerException& e) {
            ExitHelper::error("Error in Disk controller:\n%s\n", e.what());
        }
    }).detach();
}

// WRITE
MemCell& Disk::operator=(word rhs) {
    if (input_reg != 0 && !Globals::strict_flg) {
        // If strict mode is not enabled, warn when overwriting the controller input register
        throw DiskControllerException("Overwriting non-zero value in disk input register");
    }
    if (rhs > 0x1FF && !Globals::strict_flg) {
        // If strict mode is not enabled, warn when written value is more than 9-bit long
        throw DiskControllerException("Value written in Disk is bigger than 9 bit and will be truncated");
    }
    input_reg = (rhs & 0x1FF) | BUSY_BIT;
    
    return *this;
}

// READ
Disk::operator word() const {
    assert(output_reg <= 0x1FF);
    // The read value contains the busy bit from the input register
    return output_reg | (input_reg & BUSY_BIT);
}
