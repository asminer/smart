
/*
    Lexer test
*/

#include "config.h"
#include "../Streams/streams.h"
#include "../Options/optman.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/startup.h"
#include "lexer.h"

// ============================================================

class first_init : public initializer {
    public:
        first_init(exprman* em);
        virtual bool execute();
    private:
        exprman* hold_em;
};

// ============================================================

first_init::first_init(exprman* _em)
    : initializer("first_init")
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

void InitOptions(option_manager* om)
{
    if (0==om)  return;
    om->addChecklistOption("Debug",
      "Switches to control what low-level debugging information, if any, is written to the report stream."
    );
}

// ============================================================

int Usage(exprman* em)
{
    if (0==em) return 1;
    DisplayStream& cout = em->cout();
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
    io_environ myio;
    CatchSignals(&myio);

    // Options
    option_manager* om = MakeOptionManager();
    InitOptions(om);

    // Expressions
    exprman* em = Initialize_Expressions(&myio, om);

    // Bootstrap initializers, and run them
    first_init the_first_init(em);
    if ( ! initializer::executeAll() ) {
        if (em->startInternal(__FILE__, __LINE__)) {
            em->cerr() << "Deadlock in initializers";
            em->stopIO();
        }
        return -1;
    }

    // Lexer initialization
    lexer LEX(em, argv+1, argc-1);

    // finalize expression manager
    em->finalize();

    // Strip off tokens
    token t;
    do {
        LEX.consume(t);
        t.debug(em->cout());
        em->cout() << "\n";
    } while (t);

    //
    // Cleanup
    //
    delete om;
    destroyExpressionManager(em);

    return 0;
}
