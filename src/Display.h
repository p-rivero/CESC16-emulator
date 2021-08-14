#include "Terminal.h"
#include <thread>

class Display : public MemCell {
private:
    word busy_flag = 0;
    Terminal *term;

public:
    Display();

    // WRITE
    virtual MemCell& operator=(word rhs);
    // READ
    virtual operator int() const;
};
