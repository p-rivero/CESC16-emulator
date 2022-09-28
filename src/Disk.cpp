#include "Disk.h"
#include "Exceptions/EmulatorException.h"
#include "Utilities/Assert.h"
#include "Utilities/ExitHelper.h"

#include <thread>


// DISK CONTROLLER

DiskController::DiskController(volatile word *input_reg, volatile word *output_reg) : input(input_reg), output(output_reg) {
    assert(input_reg != nullptr);
    assert(output_reg != nullptr);
}

void DiskController::main_loop() {
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
        
            case Disk::ACK: throw EmulatorException("Unexpected ACK instead of command");
            default: throw EmulatorException("Unrecognized command");
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        clear(); // Commend has been processed: clear the busy bit
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
    // Acquire exit lock to prevent segfault when the main thread is exiting
    std::scoped_lock<std::mutex> lock(ExitHelper::exit_mutex);
    
    assert(data <= 0x1FF);
    *output = data;
}

// Clear the input register
void DiskController::clear() {
    // Acquire exit lock to prevent segfault when the main thread is exiting
    std::scoped_lock<std::mutex> lock(ExitHelper::exit_mutex);
    
    *input = 0;
}

void DiskController::expectAck() {
    word in = read();
    if (in != Disk::ACK)
        throw EmulatorException("Disk controller expected an ACK");
}


void DiskController::checkFileIsOpen(const std::string& funct) const {
    if (!file_is_open)
        throw EmulatorException(funct + " was called but no file was open");
}
void DiskController::checkSetFileName(const std::string& funct) const {
    if (currentFile.empty())
        throw EmulatorException(funct + " was called without using setFileName first");
}



// Disk operations

void DiskController::setFileName() {
    
}

void DiskController::openFile() {
    checkSetFileName("openFile");
    
    
}

void DiskController::closeFile() {
    checkFileIsOpen("closeFile");
    
    // Todo: close file
    file_is_open = false;
}

void DiskController::deleteFile() {
    checkSetFileName("deleteFile");
    
    
}

void DiskController::readFile() {
    // Called readRaw() on CH376 library
    checkFileIsOpen("readFile");
    
    
}

void DiskController::writeFile() {
    checkFileIsOpen("writeFile");
    
    
}

void DiskController::moveFileCursor() {
    checkFileIsOpen("moveFileCursor");
    
    
}

void DiskController::getFileCursor() {
    checkFileIsOpen("getFileCursor");
    
    
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
    std::thread([this]() {
        try {
            DiskController controller(&input_reg, &output_reg);
            controller.main_loop();
        }
        catch (const EmulatorException& e) {
            ExitHelper::error("Error in Disk controller:\n%s\n", e.what());
        }
    }).detach();
}

// WRITE
MemCell& Disk::operator=(word rhs) {
    if (input_reg != 0 && !Globals::strict_flg) {
        // If strict mode is not enabled, warn when overwriting the controller input register
        throw EmulatorException("Overwriting non-zero value in disk input register");
    }
    if (rhs > 0x1FF && !Globals::strict_flg) {
        // If strict mode is not enabled, warn when written value is more than 9-bit long
        throw EmulatorException("Value written in Disk is bigger than 9 bit and will be truncated");
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
