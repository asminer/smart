
#include "outstream.h"


int main()
{
    outputStream foo(std::cout);

    foo << "Hello, world!\n";

    foo << memoryCount(15415151132, 2);
    foo << "\n";

    outputStream::globalOut() << "The global output\n";

    return 0;

}
