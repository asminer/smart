
#include "outstream.h"


inline void left(outputStream &out, long x)
{
    out << '|' << formatted_int(x, -12, ",") << "|\n";
}

inline void right(outputStream &out, long x)
{
    out << '|' << formatted_int(x, 12, ",") << "|\n";
}

int main()
{
    outputStream foo(std::cout);

    foo << "Hello, world!\n";

    foo << memoryCount(15415151132, 2);
    foo << "\n";

    outputStream::globalOut() << "The global output\n";

    left(foo, 123);
    left(foo, 1234);
    left(foo, 12345);
    left(foo, 123456);
    left(foo, 1234567);
    left(foo, 10000008);
    left(foo, -1234567);
    left(foo, -123456);
    left(foo, -12345);
    left(foo, -1234);
    left(foo, -123);

    right(foo, 123);
    right(foo, 1234);
    right(foo, 12345);
    right(foo, 123456);
    right(foo, 1000007);
    right(foo, 12345678);
    right(foo, -1234567);
    right(foo, -100006);
    right(foo, -12345);
    right(foo, -1234);
    right(foo, -123);

    return 0;

}
