
#include "outstream.h"

inline void left(outputStream &out, long x)
{
    out << '|';
    out.putWithCommas(x, -12);
    out << "|\n";
}

inline void leftu(outputStream &out, unsigned long x)
{
    out << '|';
    out.putWithCommas(x, -12);
    out << "|\n";
}

inline void right(outputStream &out, long x)
{
    out << '|';
    out.putWithCommas(x, 12);
    out << "|\n";
}

inline void rightu(outputStream &out, unsigned long x)
{
    out << '|';
    out.putWithCommas(x, 12);
    out << "|\n";
}

int main()
{
    outputStream foo(std::cout);
    foo.comma = ",";

    foo << "Hello, world!\n";

    foo << memoryCount(15415151132, 2);
    foo << "\n";

    outputStream::globalOut() << "The global output\n";

    left(foo, 123);
    leftu(foo, 1234);
    left(foo, 12345);
    leftu(foo, 123456);
    left(foo, 1234567);
    leftu(foo, 10000008);
    left(foo, -1234567);
    left(foo, -123456);
    left(foo, -12345);
    left(foo, -1234);
    left(foo, -123);

    rightu(foo, 123);
    right(foo, 1234);
    rightu(foo, 12345);
    right(foo, 123456);
    rightu(foo, 1000007);
    right(foo, 12345678);
    right(foo, -1234567);
    right(foo, -100006);
    right(foo, -12345);
    right(foo, -1234);
    right(foo, -123);

    return 0;

}
