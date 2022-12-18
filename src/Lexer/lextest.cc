
/*
    Lexer test
*/

#include "config.h"
#include "../Options/optman.h"
#include "../Utils/initializer.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/startup.h"
#include "lexer.h"

// ============================================================

class first_init : public startup {
    public:
        first_init(exprman* em);
        virtual bool execute();
    private:
        exprman* hold_em;
};

// ============================================================

first_init::first_init(exprman* _em)
    : startup("first_init")
{
    buildsResource("em");
    hold_em = _em;
}

bool first_init::execute()
{
    em = hold_em;

    DCASSERT(em);
    return true;
}

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

    // Options
    option_manager* om = getGlobalOptionManager();

    // Expressions
    exprman* em = Initialize_Expressions(om);

    // Bootstrap startups, and run them
    first_init the_first_init(em);
    if ( ! startup::executeAll() ) {
        internal_error E(__FILE__, __LINE__);
        E << "Deadlock in startups";
        return -1;
    }

    // Run initializers
    initializer::execute_all();

    // Lexer initialization
    lexer LEX(em, argv+1, argc-1);

    // finalize expression manager
    em->finalize();

    // Strip off tokens
    token t;
    do {
        LEX.consume(t);
        t.debug(outputStream::globalOut());
        outputStream::globalOut() << "\n";
    } while (t);

    //
    // Cleanup
    //
    destroyExpressionManager(em);

    return 0;
}
