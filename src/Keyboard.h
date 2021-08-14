#include "Terminal.h"

class Keyboard : public MemCell {
private:
    Terminal *term;

    // Constants for the keyboard interface
    static const byte ACK = 0x06;
    static const byte RDY = 0x07;

public:
    Keyboard();

    // WRITE
    virtual MemCell& operator=(word rhs);
    // READ
    virtual operator int() const;
};
