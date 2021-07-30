#include "Disk.h"

// DISK CONTROLLER

DiskController::DiskController(volatile word *input_reg, volatile word *output_reg) {
    assert(input_reg != NULL);
    assert(output_reg != NULL);
    
    // Initialize data structures
    currentFile = "";
    input = input_reg;
    output = output_reg;
}

void DiskController::main_loop() {
    while (true) {
        word cmd = read();
        
        switch (cmd) {
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
        
            case Disk::ACK: throw "Unexpected ACK instead of command";
            default: throw "Unrecognized command";
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        clear(); // Commend has been processed: clear the busy bit
    }
}

// Read the input register
word DiskController::read() {
    word data = 0;
    // Poll busy bit until an input is detected
    while (not (data & Disk::BUSY_BIT)) {
        _KILL_GUARD
        data = *input;
    }
    assert(data <= 0x3FF);
    
    // Don't return the busy bit
    return data & 0x1FF;
}

// Write to the output register
void DiskController::write(word data) {
    _KILL_GUARD
    assert(data <= 0x1FF);
    *output = data;
}

// Clear the input register
void DiskController::clear() {
    _KILL_GUARD
    *input = 0;
}

void DiskController::expectAck() {
    word in = read();
    if (in != Disk::ACK)
        throw "Disk controller expected an ACK";
}


// Disk operations

void DiskController::setFileName() {
    
}

void DiskController::openFile() {
    if (currentFile.empty())
        throw "openFile() was called without using setFileName() first";
    
    
}

void DiskController::closeFile() {
    if (not file_is_open)
        throw "closeFile() was called but no file was open";
    
    // Todo: close file
    file_is_open = false;
}

void DiskController::deleteFile() {
    if (currentFile.empty())
        throw "deleteFile() was called without using setFileName() first";
    
    
}

void DiskController::readFile() {
    // Called readRaw() on CH376 library
    if (not file_is_open)
        throw "readFile() was called but no file was open";
    
    
}

void DiskController::writeFile() {
    if (not file_is_open)
        throw "writeFile() was called but no file was open";
    
    
}

void DiskController::moveFileCursor() {
    if (not file_is_open)
        throw "moveFileCursor() was called but no file was open";
    
    
}

void DiskController::getFileCursor() {
    if (not file_is_open)
        throw "getFileCursor() was called but no file was open";
    
    
}

void DiskController::listDir() {
    
}

void DiskController::cd() {
    
}

void DiskController::mkdir() {
    
}

void DiskController::getInfo() {
    
}



// DISK PERIPHERAL

Disk::Disk() {
    input_reg = output_reg = 0;
    
    std::thread([this]() {
        try {
            DiskController controller(&input_reg, &output_reg);
            controller.main_loop();
        }
        catch (const char* msg) {
            _KILL_GUARD
            destroy_terminal();
            fprintf(stderr, "Error in Disk controller:\n%s\n", msg);
            exit(EXIT_FAILURE);
        }
    }).detach();
}

// WRITE
MemCell& Disk::operator=(word rhs) {
    if (input_reg != 0 and not Globals::strict_flg) {
        // If strict mode is not enabled, warn when overwriting the controller input register
        throw "Overwriting non-zero value in disk input register";
    }
    if (rhs > 0x1FF and not Globals::strict_flg) {
        // If strict mode is not enabled, warn when written value is more than 9-bit long
        throw "Value written in Disk is bigger than 9 bit and will be truncated";
    }
    input_reg = (rhs & 0x1FF) | BUSY_BIT;
    
    return *this;
}

// READ
Disk::operator int() const {
    assert(output_reg <= 0x1FF);
    // The read value contains the busy bit from the input register
    return output_reg | (input_reg & BUSY_BIT);
}
