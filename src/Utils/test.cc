
#include "outstream.h"


int main()
{
    outputStream foo(std::cout);

    foo << "Hello, world!\n";

    unsigned char a = 'a';
    foo.PutHex(a);
    foo << " for a\n";

    unsigned b = 123456;
    foo.PutHex(b);
    foo << " for b\n";

    unsigned long c = ~0;
    foo.PutHex(c);
    foo << " for c\n";

    b = c;
    foo.PutHex(b);
    foo << "\n";

    foo.PutMemoryCount(15415151132, 2);
    foo << "\n";

    return 0;

}
