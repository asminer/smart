
/*
    Lexer test
*/

#include "config.h"
#include "../Utils/initializer.h"
#include "lexer.h"

// ============================================================

int Usage()
{
    using namespace std;
    cout << "\n";
    cout << "Usage : \n";
    cout << "lextest <file1> <file2> ... <filen>\n";
    cout << "      Use the filename `-' to denote standard input\n";
    cout << "\n";
    return 0;
}

// ============================================================

int main(int argc, const char** argv)
{
    if (1==argc) return Usage();

    // Run initializers
    initializer::execute_all();

    // Lexer initialization
    lexer LEX(argv+1, argc-1);

    // Strip off tokens
    token t;
    do {
        LEX.consume(t);
        t.debug(outputStream::globalOut());
        outputStream::globalOut() << "\n";
    } while (t);

    return 0;
}
